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
		Session* session = new Session(gUniqueID++, clientSock);
		gSessionMap.insert({ session->sessionID, session });

		// ���ϰ� ����� �Ϸ� ��Ʈ ����
		CreateIoCompletionPort((HANDLE)session->sock, hcp, (ULONG_PTR)session, 0);

		// �񵿱� ����� ����
		WSABUF wsabuf;
		wsabuf.buf = session->recvQ.GetRearBufferPtr();
		wsabuf.len = session->recvQ.GetFreeSize();
		flags = 0;
		retval = WSARecv(clientSock, &wsabuf, 1, nullptr, &flags, &session->recvOverlapped, nullptr);

		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				return -1;
			}
			continue;
		}

		OnAccept(session->sessionID);
	}

	return 0;
}

unsigned int WINAPI WorkerThread(LPVOID lpParam)
{
	int retval;
	HANDLE hcp = (HANDLE)lpParam;
	WCHAR IP[16];

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
			// ����Ʈ��ŭ Rear ����
			session->recvQ.MoveRear(Transferred);

			// �޽��� ����
			Packet packet;
			retval = session->recvQ.Dequeue(packet.GetBufferPtr(), Transferred);
			packet.MoveWritePos(retval);

			// Recv �Ŵ� �Լ�
			RecvPost(session);

			// �޼��� ó���ϴ� �Լ�
			OnRecv(session, packet);
		}
		
		// 4. Send IO
		if (overlapped == &(session->sendOverlapped))
		{
			session->sendQ.MoveFront(Transferred);
		}
		
		// ���� �����۱����� ���� ���� ����.
	}

	return 0;
}

void OnAccept(__int64 sessionID)
{
	Session* session = gSessionMap[sessionID];

	// �������� �����Ƿ� ���� �� �� ����.
}

void OnRecv(Session* session, Packet& packet)
{
	// ������ �ڵ�
	// Ŭ���̾�Ʈ ���� ���
	SOCKADDR_IN clientAddr;
	WCHAR IP[16];

	int addrLen = sizeof(clientAddr);
	getpeername(session->sock, (SOCKADDR*)&clientAddr, &addrLen);

	InetNtop(AF_INET, &(clientAddr.sin_addr), IP, 16);
	wprintf(L"[TCP/%s:%d] ", IP, ntohs(clientAddr.sin_port));

	char ch = '\0';
	packet.PutData(&ch, 1);
	printf("%s\n", packet.GetBufferPtr());
	packet.MoveWritePos(-1);

	// �ٽ� ������
	// SendPacket
	SendPacket(session->sessionID, packet);
}

void RecvPost(Session* session)
{
	int retval;
	DWORD flags = 0;
	WSABUF recvWsabuf[2];

	// Send ���� �����Ƿ�, Recv �ؾ���.
	recvWsabuf[0].buf = session->recvQ.GetRearBufferPtr();
	recvWsabuf[0].len = session->recvQ.DirectEnqueueSize();
	recvWsabuf[1].buf = session->recvQ.GetStartBufferPtr();
	recvWsabuf[1].len = session->recvQ.GetFreeSize() - recvWsabuf[0].len;

	retval = WSARecv(session->sock, &recvWsabuf[0], 2, nullptr, &flags, &(session->recvOverlapped), nullptr);

	if (retval == SOCKET_ERROR)
	{
		retval = WSAGetLastError();
		if (retval != ERROR_IO_PENDING)
		{
			printf("[WSARecv Error] Error Code : %d\n", retval);
		}
	}
}

void SendPost(Session* session)
{
	int retval;
	DWORD flags = 0;
	WSABUF sendWsabuf[2];

	// SendQ ���� �ش� �κ� ����
	sendWsabuf[0].buf = session->sendQ.GetFrontBufferPtr();
	sendWsabuf[0].len = session->sendQ.DirectDequeueSize();
	sendWsabuf[1].buf = session->sendQ.GetStartBufferPtr();
	sendWsabuf[1].len = session->sendQ.GetUseSize() - sendWsabuf[0].len;

	retval = WSASend(session->sock, &sendWsabuf[0], 2, nullptr, flags, &(session->sendOverlapped), nullptr);
	
	if (retval == SOCKET_ERROR)
	{
		retval = WSAGetLastError();
		if (retval != ERROR_IO_PENDING)
		{
			printf("[WSASend Error] Error Code : %d\n", retval);
		}
	}
}

void SendPacket(__int64 sessionID, Packet& packet)
{
	Session* session = gSessionMap[sessionID];
	session->sendQ.Enqueue(packet.GetBufferPtr(), packet.GetDataSize());

	SendPost(session);
}
