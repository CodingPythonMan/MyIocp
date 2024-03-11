#include "LanServer.h"

LanServer::LanServer()
{
	_hcp = INVALID_HANDLE_VALUE;
    _WorkerThreads = nullptr;
    _listenSock = INVALID_SOCKET;
	// Critical Section 초기화
	InitializeCriticalSection(&_mapCs);
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
    DeleteCriticalSection(&_mapCs);

    // 스레드 정리 필요
    WSACleanup();
}

int LanServer::GetSessionCount()
{
    return static_cast<int>(_SessionMap.size());
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
	WSABUF sendWsabuf[2];

	int UseSize = session->sendQ.GetUseSize();
	int DirectSize = session->sendQ.DirectDequeueSize();

	// SendQ 에서 해당 부분 설정
	sendWsabuf[0].buf = session->sendQ.GetFrontBufferPtr();

	if (UseSize > DirectSize)
	{
		sendWsabuf[0].len = DirectSize;
		sendWsabuf[1].buf = session->sendQ.GetStartBufferPtr();
		sendWsabuf[1].len = UseSize - sendWsabuf[0].len;

		retval = WSASend(session->sock, sendWsabuf, 2, NULL, flags, &session->sendOverlapped, nullptr);
	}
	else
	{
		sendWsabuf[0].len = UseSize;

		retval = WSASend(session->sock, sendWsabuf, 1, NULL, flags, &session->sendOverlapped, nullptr);
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

bool LanServer::Disconnect(SessionID sessionID)
{
    return false;
}

bool LanServer::SendPacket(SessionID sessionID, Packet& packet)
{
	Session* session = _SessionMap[sessionID];
	session->sendQ.Enqueue(packet.GetBufferPtr(), packet.GetDataSize());

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
	EnterCriticalSection(&_mapCs);
	Session* session = _SessionMap[sessionID];
	LeaveCriticalSection(&_mapCs);

	return session;
}

unsigned int WINAPI LanServer::AcceptThread(LPVOID lpParam)
{
    LanServer* server = (LanServer*)lpParam;

	int retval;

	// 데이터 통신에 사용할 변수
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrLen;
	DWORD flags;
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

		// 소켓 정보 구조체 할당
		Session* session = new Session(server->_UniqueID++, clientSock);

		EnterCriticalSection(&server->_mapCs);
		server->_SessionMap.insert({ session->sessionID, session });
		LeaveCriticalSection(&server->_mapCs);

		// 소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)session->sock, server->_hcp, (ULONG_PTR)session, 0);

		// IOCount 증가
		InterlockedIncrement(&session->IOCount);

		// 비동기 입출력 시작
		WSABUF wsabuf;
		wsabuf.buf = session->recvQ.GetRearBufferPtr();
		wsabuf.len = session->recvQ.GetFreeSize();
		flags = 0;

		// 현재 실험 중.
		retval = WSARecv(clientSock, &wsabuf, 1, NULL, &flags, &session->recvOverlapped, nullptr);

		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				// 이 경우가 아닌 경우가 무슨 경우가 있을까...?
				_LOG(LOG_LEVEL_ERROR, L"[AcceptThread] Recv Error!\n");
				return -1;
			}
			// IOPending 시 수신 버퍼에 아무것도 없다는 것.
			// 아무것도 없을 수 있다. 정상적으로 처리 되어야 한다.
		}

		// 여기로 왔다는 건 동기 처리가 되었으나 그래도 완료통지는 보내진다.

		server->OnAccept(session->sessionID);

		// 모니터링 용 증가
		server->_AcceptTPS++;
		server->_TotalAccept++;
	}

    return 0;
}

unsigned int WINAPI LanServer::WorkerThread(LPVOID lpParam)
{
	LanServer* server = (LanServer*)lpParam;

	int retval;
	WCHAR IP[16];

	while (1)
	{
		// 비동기 입출력 완료 기다리기
		DWORD Transferred;
		Session* session = NULL;
		OVERLAPPED* overlapped;
		retval = GetQueuedCompletionStatus(server->_hcp, &Transferred,
			(PULONG_PTR)&session, &overlapped, INFINITE);

		// 클라이언트 정보 얻기
		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(clientAddr);
		getpeername(session->sock, (SOCKADDR*)&clientAddr, &addrLen);

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

		// session 을 쓸 때 잠그고 생각한다.
		EnterCriticalSection(&session->cs);

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

			// 이건 차차 해결하자.
			//if(session->recvQ.Peek())

			// 메시지 저장
			Packet packet(Transferred);
			retval = session->recvQ.Dequeue(packet.GetBufferPtr(), Transferred);
			packet.MoveWritePos(retval);

			// 메세지 처리하는 함수
			server->OnRecv(session->sessionID, packet);

			// Recv 거는 함수
			server->RecvPost(session->sessionID);
		}
		// 4. Send IO
		else if (overlapped == &session->sendOverlapped)
		{
			session->sendQ.MoveFront(Transferred);

			InterlockedDecrement(&session->IOCount);
			session->WSASend = 0;

			// Dispatch Message
			if (session->sendQ.GetUseSize() > 0)
				server->SendPost(session->sessionID);
		}

		// 현재 재전송까지는 아직 넣지 않음.
		LeaveCriticalSection(&session->cs);

		if (session->IOCount == 0)
		{
			closesocket(session->sock);
			InetNtop(AF_INET, &(clientAddr.sin_addr), IP, 16);
			//wprintf(L"[TCP Server] Client Terminate : IP Address=%s, Port=%d\n",
			//	IP, ntohs(clientAddr.sin_port));

			EnterCriticalSection(&server->_mapCs);
			server->_SessionMap.erase(session->sessionID);
			LeaveCriticalSection(&server->_mapCs);

			DeleteCriticalSection(&session->cs);

			delete session;
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