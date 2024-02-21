#include "Network.h"
#include <list>
#include <process.h>
#include "Parser.h"

SOCKET gListenSock;
std::list<Session*> gSessionList;

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
	int WorkerThreadCount;
	int ConcurrentThreadCount;
	parser->LoadFile("GameConfig.ini");
	parser->GetValueInt("WorkerThreadCount", &WorkerThreadCount);
	parser->GetValueInt("ConcurrentThreadCount", &ConcurrentThreadCount);

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
	// accept �κ��� ������� �и�
	HANDLE acceptThread = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, nullptr, 0, nullptr);
	
	while (1)
	{
		// ���ϰ� ����� �Ϸ� ��Ʈ ����
		
		// �񵿱� ����� ���� �� WSARecv �� �ɾ���� ��.

	}

	WaitForSingleObject(acceptThread, INFINITE);
}

unsigned int WINAPI AcceptThread(LPVOID lpParam)
{
	// ������ ��ſ� ����� ����
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrLen;
	WCHAR IP[8];

	return 0;
}
