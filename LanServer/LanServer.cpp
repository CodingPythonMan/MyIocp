#include "LanServer.h"

LanServer::LanServer()
{
	_hcp = INVALID_HANDLE_VALUE;
    _WorkerThreads = nullptr;
    _listenSock = INVALID_SOCKET;
	
	_SessionArray = nullptr;
	InitializeCriticalSection(&_SessionIndexLock);

	_SessionCount = 0;
	_UniqueID = 0;

	// 모니터링 변수
    _AcceptTPS = 0;
	_RecvMessageTPS = 0;
	_SendMessageTPS = 0;
	_TotalAccept = 0;
}

LanServer::~LanServer()
{
}

bool LanServer::Start(WCHAR IP[], int Port, int WorkerThreadCount, bool Nagle, int MaxUserCount)
{
    int retval;

    // Log 파일 초기화
    LogFileInit();

	// 윈속 초기화
	WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        return false;
    }
	
	// 입출력 완료 포트 생성
	_hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (_hcp == nullptr)
    {
        _LOG(LOG_LEVEL_SYSTEM, L"[LanServer::Start] CreateIoCompletionPort Error!\n");
        return false;
    }

    // 리슨 소켓 셋팅
    _listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenSock == INVALID_SOCKET)
    {
        _LOG(LOG_LEVEL_SYSTEM, L"[LanServer::Start] Listen Socket Error!\n");
        return false;
    }

    // bind
    SOCKADDR_IN listenAddr;
    memset(&listenAddr, 0, sizeof(listenAddr));
    listenAddr.sin_family = AF_INET;
    InetPton(AF_INET, IP, &listenAddr.sin_addr);
    listenAddr.sin_port = htons(Port);
    retval = bind(_listenSock, (SOCKADDR*)&listenAddr, sizeof(listenAddr));
    if(retval == SOCKET_ERROR)
    {
		_LOG(LOG_LEVEL_SYSTEM, L"[LanServer::Start] Listen Socket Bind Error!\n");
		return false;
    }

    // listen()
    retval = listen(_listenSock, SOMAXCONN);
    if (retval == SOCKET_ERROR)
    {
        _LOG(LOG_LEVEL_SYSTEM, L"[LanServer::Start] Listen Socket Listen Error!\n");
        return false;
    }

	_SessionArray = new Session*[MaxUserCount];
	for (int i = 0; i < MaxUserCount; i++)
	{
		_SessionIndex.push(i);
		_SessionArray[i] = new Session();
	}

    _WorkerThreads = new HANDLE[WorkerThreadCount + 2];
    _WorkerThreads[0] = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, this, 0, nullptr);
    _WorkerThreads[1] = (HANDLE)_beginthreadex(nullptr, 0, CalTPSThread, this, 0, nullptr);
    for (int i = 0; i < WorkerThreadCount; i++)
    {
        _WorkerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, this, 0, nullptr);
    }

    return true;
}

void LanServer::Stop()
{
	delete[] _SessionArray;
	DeleteCriticalSection(&_SessionIndexLock);

    // 스레드 정리 필요
    WSACleanup();
}

int LanServer::GetSessionCount()
{
    return _SessionCount;
}

void LanServer::RecvPost(SessionID sessionID)
{
	Session* session = FindSession(sessionID);

	int retval;
	DWORD flags = 0;
	WSABUF recvWsabuf[2];

	int FreeSize = session->recvQ.GetFreeSize();
	int DirectSize = session->recvQ.DirectEnqueueSize();

	recvWsabuf[0].buf = session->recvQ.GetRearBufferPtr();
	if (FreeSize > DirectSize)
	{
		recvWsabuf[0].len = DirectSize;
		recvWsabuf[1].buf = session->recvQ.GetStartBufferPtr();
		recvWsabuf[1].len = FreeSize - DirectSize;

		retval = WSARecv(session->sock, recvWsabuf, 2, NULL, &flags, &session->recvOverlapped, nullptr);
	}
	else
	{
		recvWsabuf[0].len = FreeSize;

		retval = WSARecv(session->sock, recvWsabuf, 1, NULL, &flags, &session->recvOverlapped, nullptr);
	}

	if (retval == SOCKET_ERROR)
	{
		retval = WSAGetLastError();
		if (retval != ERROR_IO_PENDING)
		{
			return;
		}
	}
}

void LanServer::SendPost(SessionID sessionID)
{
	Session* session = FindSession(sessionID);

	// Send 전 버퍼링
	if (InterlockedExchange(&session->WSASend, 1) == 1)
		return;

	InterlockedIncrement(&session->IOCount);

	int retval;
	DWORD flags = 0;
	WSABUF sendWsabuf[125];

	int DequeueSize = session->sendQ.DirectDequeueSize();
	int UseSize = session->sendQ.GetUseSize();
	int PacketCount = DequeueSize / sizeof(Packet*);

	char* ptr = session->sendQ.GetFrontBufferPtr();

	Packet* packet;
	for (int i = 0; i < PacketCount; i++)
	{
		packet = *(((Packet**)ptr) + i);
		sendWsabuf[i].buf = packet->GetBufferPtr();
		sendWsabuf[i].len = packet->GetDataSize();
	}

	if (DequeueSize < sizeof(Packet*) && sizeof(Packet*) <= UseSize)
	{
		session->sendQ.Peek((char*)&packet, sizeof(Packet*));
		sendWsabuf[PacketCount].buf = packet->GetBufferPtr();
		sendWsabuf[PacketCount].len = packet->GetDataSize();
		PacketCount += 1;
	}

	session->SendPacket = PacketCount;

	retval = WSASend(session->sock, sendWsabuf, PacketCount, NULL, flags, &session->sendOverlapped, nullptr);

	if (retval == SOCKET_ERROR)
	{
		retval = WSAGetLastError();
		if (retval != ERROR_IO_PENDING)
		{
			return;
		}
	}
}

bool LanServer::Disconnect(SessionID sessionID)
{
	Session* session = FindSession(sessionID);

	// 클라이언트 정보 얻기
	SOCKADDR_IN clientAddr;
	WCHAR IP[16];

	int addrLen = sizeof(clientAddr);
	getpeername(session->sock, (SOCKADDR*)&clientAddr, &addrLen);
	closesocket(session->sock);
	InetNtop(AF_INET, &(clientAddr.sin_addr), IP, 16);
	//wprintf(L"[TCP Server] Client Terminate : IP Address=%s, Port=%d\n",
	//	IP, ntohs(clientAddr.sin_port));

	unsigned short index = ConvertIndex(sessionID);

	EnterCriticalSection(&_SessionIndexLock);
	_SessionIndex.push(index);
	LeaveCriticalSection(&_SessionIndexLock);

    return true;
}

bool LanServer::SendPacket(SessionID sessionID, Packet* packet)
{
	Session* session = FindSession(sessionID);
	session->sendQ.Enqueue((char*)&packet, sizeof(Packet*));

	InterlockedIncrement((long*)&_SendMessageTPS);

	SendPost(session->sessionID);

    return false;
}

int LanServer::GetAcceptTPS()
{
    return _AcceptTPS;        
}

int LanServer::GetRecvMessageTPS()
{
    return _RecvMessageTPS;
}

int LanServer::GetSendMessageTPS()
{
    return _SendMessageTPS;
}

Session* LanServer::FindSession(SessionID sessionID)
{
	unsigned short index = ConvertIndex(sessionID);

	Session* session = _SessionArray[index];

	return session;
}

unsigned short LanServer::ConvertIndex(SessionID sessionID)
{
	return sessionID >> 48;
}

unsigned int WINAPI LanServer::AcceptThread(LPVOID lpParam)
{
    LanServer* server = (LanServer*)lpParam;

	// 데이터 통신에 사용할 변수
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrLen;
	WCHAR IP[16];

	while (1)
	{
		// accept
		addrLen = sizeof(clientAddr);
		clientSock = accept(server->_listenSock, (SOCKADDR*)&clientAddr, &addrLen);

		if (clientSock == INVALID_SOCKET)
		{
            _LOG(LOG_LEVEL_ERROR, L"[AcceptThread] Client Accept Error!\n");
            continue;
		}

		InetNtop(AF_INET, &(clientAddr.sin_addr), IP, 16);
		//_LOG(LOG_LEVEL_DEBUG, L"[TCP Server] Client Access : IP Address=%s, Port=%d\n",
		//	IP, ntohs(clientAddr.sin_port));

		// Array 에서 안 쓰는 Session 인덱스를 찾은 후, 그 인덱스 + UniqueID 합성해야한다.
		EnterCriticalSection(&server->_SessionIndexLock);
		SessionID sessionID = server->_SessionIndex.top();
		server->_SessionIndex.pop();
		LeaveCriticalSection(&server->_SessionIndexLock);

		Session* session = server->_SessionArray[sessionID];
		sessionID = (sessionID << 48) + server->_UniqueID;
		server->_UniqueID++;

		if (server->_UniqueID >= MAX_UNIQUE_ID)
			server->_UniqueID = 0;

		session->sock = clientSock;
		session->sessionID = sessionID;
		session->sendQ.ClearBuffer();
		session->recvQ.ClearBuffer();
		
		// 세션 증가
		InterlockedIncrement(&server->_SessionCount);

		// 소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)session->sock, server->_hcp, (ULONG_PTR)session, 0);

		// IOCount 증가
		InterlockedIncrement(&session->IOCount);

		server->RecvPost(session->sessionID);

		server->OnAccept(session->sessionID);

		// 모니터링 용 증가
		InterlockedIncrement((long*)&server->_AcceptTPS);
		InterlockedIncrement((long*)&server->_TotalAccept);
	}

    return 0;
}

unsigned int WINAPI LanServer::WorkerThread(LPVOID lpParam)
{
	LanServer* server = (LanServer*)lpParam;

	int retval;

	while (1)
	{
		// 비동기 입출력 완료 기다리기
		DWORD Transferred;
		Session* session = NULL;
		OVERLAPPED* overlapped;
		retval = GetQueuedCompletionStatus(server->_hcp, &Transferred,
			(PULONG_PTR)&session, &overlapped, INFINITE);

		// 비동기 입출력 결과 확인
		// 1. 3개 값 0 확인
		if (session == 0 && overlapped == 0 && Transferred == 0)
		{
			// 지금 단계에선 일어나면 안 됨.
			__debugbreak();
			break;
		}

		if (session == nullptr)
		{
			printf("[WorkerThread Error] Session is null!");
			break;
		}

		// 2. Transferred == 0 확인 ( Send 나 Recv 의 오류 ) 
		if (Transferred == 0)
		{
			// IO Count 감소
			InterlockedDecrement(&session->IOCount);
		}
		// 3. Recv IO 인지 확인
		else if (overlapped == &session->recvOverlapped)
		{
			// 바이트만큼 Rear 변경
			session->recvQ.MoveRear(Transferred);

			while (1)
			{
				if (session->recvQ.GetUseSize() <= HEADER_SIZE)
					break;

				unsigned short len;
				session->recvQ.Peek((char*)&len, sizeof(len));
				if (session->recvQ.GetUseSize() < HEADER_SIZE + len)
					break;

				// 메시지 저장
				Packet packet(HEADER_SIZE + len);
				retval = session->recvQ.Dequeue(packet.GetBufferPtr(), HEADER_SIZE + len);
				packet.MoveWritePos(retval);

				// 메세지 처리하는 함수
				server->OnRecv(session->sessionID, packet);
			}

			// Recv 거는 함수
			server->RecvPost(session->sessionID);
		}
		// 4. Send IO
		else if (overlapped == &session->sendOverlapped)
		{
			Packet* packet;
			for (int i = 0; i < session->SendPacket; i++)
			{
				session->sendQ.Dequeue((char*)&packet, sizeof(Packet*));
				delete packet;
			}

			session->WSASend = 0;
			session->SendPacket = 0;
			InterlockedDecrement(&session->IOCount);

			// Dispatch Message
			if (session->sendQ.GetUseSize() > 0)
				server->SendPost(session->sessionID);
		}

		if (session->IOCount == 0)
		{
			server->Disconnect(session->sessionID);
		}
	}

    return 0;
}

unsigned int WINAPI LanServer::CalTPSThread(LPVOID lpParam)
{
    LanServer* server = (LanServer*)lpParam;

    while (1)
    {
        _LOG(LOG_LEVEL_DEBUG, L"TotalAccept : %d", server->_TotalAccept);
        _LOG(LOG_LEVEL_DEBUG, L"AcceptTPS : %d", server->_AcceptTPS);
        _LOG(LOG_LEVEL_DEBUG, L"RecvMessageTPS : %d", server->_RecvMessageTPS);
        _LOG(LOG_LEVEL_DEBUG, L"SendMessageTPS : %d", server->_SendMessageTPS);

        server->_AcceptTPS = 0;
        server->_RecvMessageTPS = 0;
        server->_SendMessageTPS = 0;

        Sleep(1000);
    }

    return 0;
}