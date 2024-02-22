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

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return;

	// 입출력 완료 포트 생성
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (hcp == nullptr)
		return;

	// 스레드 설계된대로 실행한다.
	Parser* parser = Parser::GetInstance();
	
	int ConcurrentThreadCount;
	parser->LoadFile("GameConfig.ini");
	parser->GetValueInt("WorkerThreadCount", &WorkerThreadCount);
	parser->GetValueInt("ConcurrentThreadCount", &ConcurrentThreadCount);

	// 스레드 실행.
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

	// 데이터 통신에 사용할 변수
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

		// 소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)clientSock, hcp, clientSock, 0);

		// 소켓 정보 구조체 할당
		Session* ptr = new Session;

		memset(&ptr->recvOverlapped, 0, sizeof(ptr->recvOverlapped));
		memset(&ptr->sendOverlapped, 0, sizeof(ptr->sendOverlapped));
		ptr->sock = clientSock;

		// 비동기 입출력 시작
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
		// 비동기 입출력 완료 기다리기
		DWORD Transferred;
		SOCKET clientSock;
		Session* ptr;
		retval = GetQueuedCompletionStatus(hcp, &Transferred, 
			&clientSock, (LPOVERLAPPED*)&ptr, INFINITE);

		// 클라이언트 정보 얻기
		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(clientAddr);
		getpeername(ptr->sock, (SOCKADDR*)&clientAddr, &addrLen);

		// 비동기 입출력 결과 확인
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
