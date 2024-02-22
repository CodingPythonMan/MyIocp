#include "Network.h"
#include <list>
#include <process.h>
#include "Parser.h"

SOCKET gListenSock;
std::list<Session*> gSessionList;

HANDLE* Threads;
int WorkerThreadCount;

void NetworkInit()
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return;

	// ����� �Ϸ� ��Ʈ ����
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (hcp == nullptr)
		return;

	// ������ ����ȴ�� �����Ѵ�.
	Parser* parser = Parser::GetInstance();
	
	int ConcurrentThreadCount;
	parser->LoadFile("GameConfig.ini");
	parser->GetValueInt("WorkerThreadCount", &WorkerThreadCount);
	parser->GetValueInt("ConcurrentThreadCount", &ConcurrentThreadCount);

	// ������ ����.
	Threads = new HANDLE[WorkerThreadCount];
	for (int i = 0; i < WorkerThreadCount - 1; i++)
	{
		Threads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, hcp, 0, nullptr);
	}
	Threads[WorkerThreadCount-1] = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, hcp, 0, nullptr);

	// socket()
	gListenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (gListenSock == INVALID_SOCKET)
		return;

	// bind()
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(SERVERPORT);
	retval = bind(gListenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR)
		return;

	// listen()
	retval = listen(gListenSock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		return;
}

void NetworkIO()
{
	NetworkInit();

	WaitForMultipleObjects(WorkerThreadCount, Threads, true, INFINITE);
	delete[] Threads;
}

unsigned int WINAPI AcceptThread(LPVOID lpParam)
{
	int retval;

	// ������ ��ſ� ����� ����
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrLen;
	DWORD flags;
	WCHAR IP[8];

	HANDLE hcp = (HANDLE)lpParam;

	while (1)
	{
		// accept
		addrLen = sizeof(clientAddr);
		clientSock = accept(gListenSock, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSock == INVALID_SOCKET)
		{
			printf("[Accept Thread] : Client Sock Invalid Error!\n");
			break;
		}

		InetNtop(AF_INET, &(clientAddr.sin_addr), IP, 8);
		wprintf(L"[TCP Server] Client Access : IP Address=%s, Port=%d\n",
			IP, ntohs(clientAddr.sin_port));

		// ���ϰ� ����� �Ϸ� ��Ʈ ����
		CreateIoCompletionPort((HANDLE)clientSock, hcp, clientSock, 0);

		// ���� ���� ����ü �Ҵ�
		Session* ptr = new Session;

		memset(&ptr->recvOverlapped, 0, sizeof(ptr->recvOverlapped));
		memset(&ptr->sendOverlapped, 0, sizeof(ptr->sendOverlapped));
		ptr->sock = clientSock;

		// �񵿱� ����� ����
		WSABUF wsabuf;
		flags = 0;
		retval = WSARecv(clientSock, &wsabuf, 1, nullptr, &flags, &ptr->recvOverlapped, nullptr);

		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				return -1;
			}
			continue;
		}

	}

	return 0;
}

unsigned int WINAPI WorkerThread(LPVOID lpParam)
{
	int retval;
	HANDLE hcp = (HANDLE)lpParam;

	WCHAR IP[8];

	while (1)
	{
		// �񵿱� ����� �Ϸ� ��ٸ���
		DWORD Transferred;
		SOCKET clientSock;
		Session* ptr;
		retval = GetQueuedCompletionStatus(hcp, &Transferred, 
			&clientSock, (LPOVERLAPPED*)&ptr, INFINITE);

		// Ŭ���̾�Ʈ ���� ���
		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(clientAddr);
		getpeername(ptr->sock, (SOCKADDR*)&clientAddr, &addrLen);

		// �񵿱� ����� ��� Ȯ��
		if (retval == 0 || Transferred == 0)
		{
			if (retval == 0)
			{
				DWORD temp1, temp2;
			}
			closesocket(ptr->sock);
			InetNtop(AF_INET, &(clientAddr.sin_addr), IP, 16);
			wprintf(L"[TCP Server] Client Terminate : IP Address=%s, Port=%d\n",
				IP, ntohs(clientAddr.sin_port));
			delete ptr;
			continue;
		}
	}

	return 0;
}
