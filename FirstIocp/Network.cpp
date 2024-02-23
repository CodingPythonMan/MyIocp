#include "Network.h"
#include <process.h>
#include "Parser.h"
#include <iostream>
#include "Packet.h"
#include <unordered_map>

SOCKET gListenSock;

HANDLE* gThreads;
int gWorkerThreadCount;

__int64 gUniqueID = 0;
std::unordered_map<__int64, Session*> gSessionMap;

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
	parser->GetValueInt("WorkerThreadCount", &gWorkerThreadCount);
	parser->GetValueInt("ConcurrentThreadCount", &ConcurrentThreadCount);

	// ������ ����.
	gThreads = new HANDLE[gWorkerThreadCount];
	for (int i = 0; i < gWorkerThreadCount - 1; i++)
	{
		gThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, hcp, 0, nullptr);
	}
	gThreads[gWorkerThreadCount-1] = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, hcp, 0, nullptr);

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

	WaitForMultipleObjects(gWorkerThreadCount, gThreads, true, INFINITE);
	delete[] gThreads;
}

unsigned int WINAPI AcceptThread(LPVOID lpParam)
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
		clientSock = accept(gListenSock, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSock == INVALID_SOCKET)
		{
			printf("[Accept Thread] : Client Sock Invalid Error!\n");
			break;
		}

		InetNtop(AF_INET, &(clientAddr.sin_addr), IP, 16);
		wprintf(L"[TCP Server] Client Access : IP Address=%s, Port=%d\n",
			IP, ntohs(clientAddr.sin_port));

		// ���� ���� ����ü �Ҵ�
		Session* ptr = new Session(clientSock);
		gSessionMap.insert({ gUniqueID++, ptr });

		// ���ϰ� ����� �Ϸ� ��Ʈ ����
		CreateIoCompletionPort((HANDLE)clientSock, hcp, (ULONG_PTR)ptr, 0);

		// �񵿱� ����� ����
		WSABUF wsabuf;
		wsabuf.buf = ptr->recvQ.GetRearBufferPtr();
		wsabuf.len = ptr->recvQ.GetFreeSize();
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
	WCHAR IP[16];
	DWORD flags;
	WSABUF sendWsabuf[2];
	WSABUF recvWsabuf[2];

	while (1)
	{
		// �񵿱� ����� �Ϸ� ��ٸ���
		DWORD Transferred;
		Session* session = NULL;
		OVERLAPPED* overlapped;
		retval = GetQueuedCompletionStatus(hcp, &Transferred, 
			(PULONG_PTR)&session, &overlapped, INFINITE);

		// Ŭ���̾�Ʈ ���� ���
		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(clientAddr);
		getpeername(session->sock, (SOCKADDR*)&clientAddr, &addrLen);

		// �񵿱� ����� ��� Ȯ��
		// 1. 3�� �� 0 Ȯ��
		if (session == 0 && overlapped == 0 && Transferred == 0)
		{
			break;
		}

		if (session == nullptr)
		{
			printf("[WorkerThread Error] Session is null!");
			break;
		}
	
		// 2. Transferred == 0 Ȯ��
		if (Transferred == 0)
		{
			closesocket(session->sock);
			InetNtop(AF_INET, &(clientAddr.sin_addr), IP, 16);
			wprintf(L"[TCP Server] Client Terminate : IP Address=%s, Port=%d\n",
				IP, ntohs(clientAddr.sin_port));
			
			delete session;
			continue;
		}

		// 3. Recv IO ���� Ȯ��
		if (overlapped == &(session->recvOverlapped))
		{
			session->recvQ.MoveRear(Transferred);

			// �޾Ҵٸ�, SendQ�� ����
			InetNtop(AF_INET, &(clientAddr.sin_addr), IP, 16);
			wprintf(L"[TCP/%s:%d] ", IP, ntohs(clientAddr.sin_port));

			char* message = new char[Transferred + 1];
			session->recvQ.Dequeue(message, Transferred);
			message[Transferred] = '\0';
			printf("%s\n", message);
			
			session->sendQ.Enqueue(message, Transferred);
			delete[] message;

			// SendQ ���� �ش� �κ� ����
			sendWsabuf[0].buf = session->sendQ.GetFrontBufferPtr();
			sendWsabuf[0].len = session->sendQ.DirectDequeueSize();
			flags = 0;

			retval = WSASend(session->sock, &sendWsabuf[0], 1, nullptr, flags, &(session->sendOverlapped), nullptr);
			if (retval == SOCKET_ERROR)
			{
				retval = WSAGetLastError();
				if (retval != ERROR_IO_PENDING)
				{
					printf("[WSASend Error] Error Code : %d\n", retval);
					return 1;
				}
				continue;
			}

			// Send ���� �����Ƿ�, Recv �ؾ���.
			recvWsabuf[0].buf = session->recvQ.GetRearBufferPtr();
			recvWsabuf[0].len = session->recvQ.DirectEnqueueSize();
			
			retval = WSARecv(session->sock, &recvWsabuf[0], 1, nullptr, &flags, &(session->recvOverlapped), nullptr);

			if (retval == SOCKET_ERROR)
			{
				retval = WSAGetLastError();
				if (retval != ERROR_IO_PENDING)
				{
					printf("[WSARecv Error] Error Code : %d\n", retval);
					return 1;
				}
				continue;
			}
		}
		
		// 4. Send IO
		session->sendQ.MoveFront(Transferred);

		// ���� �����۱����� ���� ���� ����.
	}

	return 0;
}
