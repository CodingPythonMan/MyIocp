#include "LanServer.h"

LanServer::LanServer()
{
	_hcp = INVALID_HANDLE_VALUE;
    _WorkerThreads = nullptr;
    _listenSock = INVALID_SOCKET;
	// Critical Section �ʱ�ȭ
	InitializeCriticalSection(&_mapCs);
	_UniqueID = 0;

	// ����͸� ����
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

    // Log ���� �ʱ�ȭ
    LogFileInit();

	// ���� �ʱ�ȭ
	WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        return false;
    }
	
	// ����� �Ϸ� ��Ʈ ����
	_hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (_hcp == nullptr)
    {
        _LOG(LOG_LEVEL_SYSTEM, L"[LanServer::Start] CreateIoCompletionPort Error!\n");
        return false;
    }

    // ���� ���� ����
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

    // ������ ���� �ʿ�
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

	// Send �� ���۸�
	if (InterlockedExchange(&session->WSASend, 1) == 1)
		return;

	InterlockedIncrement(&session->IOCount);

	int retval;
	DWORD flags = 0;
	WSABUF sendWsabuf[2];

	int UseSize = session->sendQ.GetUseSize();
	int DirectSize = session->sendQ.DirectDequeueSize();

	// SendQ ���� �ش� �κ� ����
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

	// ������ ��ſ� ����� ����
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

		// ���� ���� ����ü �Ҵ�
		Session* session = new Session(server->_UniqueID++, clientSock);

		EnterCriticalSection(&server->_mapCs);
		server->_SessionMap.insert({ session->sessionID, session });
		LeaveCriticalSection(&server->_mapCs);

		// ���ϰ� ����� �Ϸ� ��Ʈ ����
		CreateIoCompletionPort((HANDLE)session->sock, server->_hcp, (ULONG_PTR)session, 0);

		// IOCount ����
		InterlockedIncrement(&session->IOCount);

		// �񵿱� ����� ����
		WSABUF wsabuf;
		wsabuf.buf = session->recvQ.GetRearBufferPtr();
		wsabuf.len = session->recvQ.GetFreeSize();
		flags = 0;

		// ���� ���� ��.
		retval = WSARecv(clientSock, &wsabuf, 1, NULL, &flags, &session->recvOverlapped, nullptr);

		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				// �� ��찡 �ƴ� ��찡 ���� ��찡 ������...?
				_LOG(LOG_LEVEL_ERROR, L"[AcceptThread] Recv Error!\n");
				return -1;
			}
			// IOPending �� ���� ���ۿ� �ƹ��͵� ���ٴ� ��.
			// �ƹ��͵� ���� �� �ִ�. ���������� ó�� �Ǿ�� �Ѵ�.
		}

		// ����� �Դٴ� �� ���� ó���� �Ǿ����� �׷��� �Ϸ������� ��������.

		server->OnAccept(session->sessionID);

		// ����͸� �� ����
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
		// �񵿱� ����� �Ϸ� ��ٸ���
		DWORD Transferred;
		Session* session = NULL;
		OVERLAPPED* overlapped;
		retval = GetQueuedCompletionStatus(server->_hcp, &Transferred,
			(PULONG_PTR)&session, &overlapped, INFINITE);

		// Ŭ���̾�Ʈ ���� ���
		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(clientAddr);
		getpeername(session->sock, (SOCKADDR*)&clientAddr, &addrLen);

		// �񵿱� ����� ��� Ȯ��
		// 1. 3�� �� 0 Ȯ��
		if (session == 0 && overlapped == 0 && Transferred == 0)
		{
			// ���� �ܰ迡�� �Ͼ�� �� ��.
			__debugbreak();
			break;
		}

		if (session == nullptr)
		{
			printf("[WorkerThread Error] Session is null!");
			break;
		}

		// session �� �� �� ��װ� �����Ѵ�.
		EnterCriticalSection(&session->cs);

		// 2. Transferred == 0 Ȯ�� ( Send �� Recv �� ���� ) 
		if (Transferred == 0)
		{
			// IO Count ����
			InterlockedDecrement(&session->IOCount);
		}
		// 3. Recv IO ���� Ȯ��
		else if (overlapped == &session->recvOverlapped)
		{
			// ����Ʈ��ŭ Rear ����
			session->recvQ.MoveRear(Transferred);

			// �̰� ���� �ذ�����.
			//if(session->recvQ.Peek())

			// �޽��� ����
			Packet packet(Transferred);
			retval = session->recvQ.Dequeue(packet.GetBufferPtr(), Transferred);
			packet.MoveWritePos(retval);

			// �޼��� ó���ϴ� �Լ�
			server->OnRecv(session->sessionID, packet);

			// Recv �Ŵ� �Լ�
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

		// ���� �����۱����� ���� ���� ����.
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