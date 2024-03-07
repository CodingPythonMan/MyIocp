#include "LanServer.h"

LanServer::LanServer()
{
    listenSock = INVALID_SOCKET;
    WorkerThreads = nullptr;

    AcceptTPS = 0;
    RecvMessageTPS = 0;
    SendMessageTPS = 0;
}

LanServer::~LanServer()
{
}

bool LanServer::Start(WCHAR IP[], int Port, int WorkerThreadCount, bool Nagle, int MaxUserCount)
{
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
        return false;
    }



    WorkerThreads[0] = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, nullptr, 0, nullptr);
    for (int i = 1; i < WorkerThreadCount; i++)
    {
        WorkerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
    }

    return true;
}

void LanServer::Stop()
{
    // ������ ���� �ʿ�
    WSACleanup();
}

int LanServer::GetSessionCount()
{
    return 0;
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
    return 0;
}

int LanServer::GetRecvMessageTPS()
{
    return 0;
}

int LanServer::GetSendMessageTPS()
{
    return 0;
}

unsigned int WINAPI LanServer::AcceptThread(LPVOID lpParam)
{
    return 0;
}

unsigned int WINAPI LanServer::WorkerThread(LPVOID lpParam)
{
    return 0;
}
