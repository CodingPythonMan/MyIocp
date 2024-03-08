#include "LanServer.h"

SOCKET LanServer::listenSock = INVALID_SOCKET;
CRITICAL_SECTION LanServer::mapCs;
std::unordered_map<__int64, Session*> LanServer::SessionMap;
int LanServer::AcceptTPS = 0;
int LanServer::RecvMessageTPS = 0;
int LanServer::SendMessageTPS = 0;
unsigned __int64 LanServer::UniqueID = 0;

LanServer::LanServer()
{
    _WorkerThreads = nullptr;
}

LanServer::~LanServer()
{
}

bool LanServer::Start(WCHAR IP[], int Port, int WorkerThreadCount, bool Nagle, int MaxUserCount)
{
    int retval;

    // Log ���� �ʱ�ȭ
    LogFileInit();

    // Critical Section �ʱ�ȭ
    InitializeCriticalSection(&mapCs);

	// ���� �ʱ�ȭ
	WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        return false;
    }
	
	// ����� �Ϸ� ��Ʈ ����
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (hcp == nullptr)
    {
        _LOG(LOG_LEVEL_SYSTEM, L"[LanServer::Start] CreateIoCompletionPort Error!\n");
        return false;
    }

    // ���� ���� ����
    listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET)
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
    retval = bind(listenSock, (SOCKADDR*)&listenAddr, sizeof(listenAddr));
    if(retval == SOCKET_ERROR)
    {
		_LOG(LOG_LEVEL_SYSTEM, L"[LanServer::Start] Listen Socket Bind Error!\n");
		return false;
    }

    // listen()
    retval = listen(listenSock, SOMAXCONN);
    if (retval == SOCKET_ERROR)
    {
        _LOG(LOG_LEVEL_SYSTEM, L"[LanServer::Start] Listen Socket Listen Error!\n");
        return false;
    }

    _WorkerThreads = new HANDLE[WorkerThreadCount + 2];
    _WorkerThreads[0] = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, nullptr, 0, nullptr);
    _WorkerThreads[1] = (HANDLE)_beginthreadex(nullptr, 0, CalTPSThread, nullptr, 0, nullptr);
    for (int i = 0; i < WorkerThreadCount; i++)
    {
        _WorkerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
    }

    return true;
}

void LanServer::Stop()
{
    DeleteCriticalSection(&mapCs);

    // ������ ���� �ʿ�
    WSACleanup();
}

int LanServer::GetSessionCount()
{
    return SessionMap.size();
}

bool LanServer::Disconnect(SessionID sessionID)
{
    return false;
}

bool LanServer::SendPacket(SessionID sessionID, Packet* packet)
{
    return false;
}

int LanServer::GetAcceptTPS()
{
    return AcceptTPS;
}

int LanServer::GetRecvMessageTPS()
{
    return RecvMessageTPS;
}

int LanServer::GetSendMessageTPS()
{
    return SendMessageTPS;
}

unsigned int WINAPI LanServer::AcceptThread(LPVOID lpParam)
{
	int retval;

	// ������ ��ſ� ����� ����
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrLen;
	DWORD flags;
	WCHAR IP[16];

	HANDLE hcp = (HANDLE)lpParam;

	while (1)
	{
		// accept
		addrLen = sizeof(clientAddr);
		clientSock = accept(listenSock, (SOCKADDR*)&clientAddr, &addrLen);

		if (clientSock == INVALID_SOCKET)
		{
            _LOG(LOG_LEVEL_ERROR, L"[AcceptThread] Client Accept Error!\n");
            continue;
		}

		InetNtop(AF_INET, &(clientAddr.sin_addr), IP, 16);
		_LOG(LOG_LEVEL_DEBUG, L"[TCP Server] Client Access : IP Address=%s, Port=%d\n",
			IP, ntohs(clientAddr.sin_port));

		// ���� ���� ����ü �Ҵ�
		Session* session = new Session(UniqueID++, clientSock);

		EnterCriticalSection(&mapCs);
		SessionMap.insert({ session->sessionID, session });
		LeaveCriticalSection(&mapCs);

		// ���ϰ� ����� �Ϸ� ��Ʈ ����
		CreateIoCompletionPort((HANDLE)session->sock, hcp, (ULONG_PTR)session, 0);

		// �񵿱� ����� ����
		WSABUF wsabuf;
		wsabuf.buf = session->recvQ.GetRearBufferPtr();
		wsabuf.len = session->recvQ.GetFreeSize();
		flags = 0;

		retval = WSARecv(clientSock, &wsabuf, 1, NULL, &flags, &session->recvOverlapped, nullptr);

		// IOCount ����
		InterlockedIncrement(&session->IOCount);

		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				return -1;
			}
			continue;
		}

		//OnAccept(session->sessionID);
	}

    return 0;
}

unsigned int WINAPI LanServer::WorkerThread(LPVOID lpParam)
{


    return 0;
}

unsigned int WINAPI LanServer::CalTPSThread(LPVOID lpParam)
{
    while (1)
    {
        _LOG(LOG_LEVEL_DEBUG, L"AcceptTPS : %d", AcceptTPS);
        _LOG(LOG_LEVEL_DEBUG, L"RecvMessageTPS : %d", RecvMessageTPS);
        _LOG(LOG_LEVEL_DEBUG, L"SendMessageTPS : %d", SendMessageTPS);

        AcceptTPS = 0;
        RecvMessageTPS = 0;
        SendMessageTPS = 0;

        Sleep(1000);
    }

    return 0;
}