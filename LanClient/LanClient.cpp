#include "LanClient.h"
#include <conio.h>

LanClient::LanClient()
{
	_hcp = INVALID_HANDLE_VALUE;
	_WorkerThreads = nullptr;

	_session = new Session();
}

LanClient::~LanClient()
{
}

bool LanClient::Start(WCHAR IP[], int Port, int WorkerThreadCount)
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

	// coonnect ���� ����
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	InetPton(AF_INET, IP, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(Port);
	
	_session->sock = socket(AF_INET, SOCK_STREAM, 0);
	
	retval = connect(_session->sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR)
	{
		_LOG(LOG_LEVEL_SYSTEM, L"[LanClient::Connect] Connect Error!\n");
		return false;
	}

	_WorkerThreads = new HANDLE[WorkerThreadCount];
	for (int i = 0; i < WorkerThreadCount; i++)
	{
		_WorkerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, this, 0, nullptr);
	}

	// ���ϰ� ����� �Ϸ� ��Ʈ ����
	CreateIoCompletionPort((HANDLE)_session->sock, _hcp, (ULONG_PTR)_session, 0);

	RecvPost();

	// ���⿡�� Server �ʿ� WSASend �� ��Ŷ�� ������ ����.
	Packet packet;
	PACKET_HEADER header;
	header.byCode = NETWORK_CODE;
	header.bySize = 0;
	header.byType = QRY_LOGIN;
	packet.PutData((char*)&header, HEADER_SIZE);
	SendPacket(packet);

	while (1)
	{
		/*
		if (_kbhit())
		{
			WCHAR ControlKey = _getwch();

			if ()
			{
				
			}
		}*/

		Sleep(100);
	}

	return true;
}

void LanClient::Stop()
{
	// ������ ���� �ʿ�
	delete _session;

	WSACleanup();
}

void LanClient::RecvPost()
{
	int retval;
	DWORD flags = 0;
	WSABUF recvWsabuf[2];

	int FreeSize = _session->recvQ.GetFreeSize();
	int DirectSize = _session->recvQ.DirectEnqueueSize();

	recvWsabuf[0].buf = _session->recvQ.GetRearBufferPtr();
	if (FreeSize > DirectSize)
	{
		recvWsabuf[0].len = DirectSize;
		recvWsabuf[1].buf = _session->recvQ.GetStartBufferPtr();
		recvWsabuf[1].len = FreeSize - DirectSize;

		retval = WSARecv(_session->sock, recvWsabuf, 2, NULL, &flags, &_session->recvOverlapped, nullptr);
	}
	else
	{
		recvWsabuf[0].len = FreeSize;

		retval = WSARecv(_session->sock, recvWsabuf, 1, NULL, &flags, &_session->recvOverlapped, nullptr);
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

void LanClient::SendPost()
{
	// Send �� ���۸�
	if (InterlockedExchange(&_session->WSASend, 1) == 1)
		return;

	InterlockedIncrement(&_session->IOCount);

	int retval;
	DWORD flags = 0;
	WSABUF sendWsabuf[2];

	int UseSize = _session->sendQ.GetUseSize();
	int DirectSize = _session->sendQ.DirectDequeueSize();

	// SendQ ���� �ش� �κ� ����
	sendWsabuf[0].buf = _session->sendQ.GetFrontBufferPtr();

	if (UseSize > DirectSize)
	{
		sendWsabuf[0].len = DirectSize;
		sendWsabuf[1].buf = _session->sendQ.GetStartBufferPtr();
		sendWsabuf[1].len = UseSize - sendWsabuf[0].len;

		retval = WSASend(_session->sock, sendWsabuf, 2, NULL, flags, &_session->sendOverlapped, nullptr);
	}
	else
	{
		sendWsabuf[0].len = UseSize;

		retval = WSASend(_session->sock, sendWsabuf, 1, NULL, flags, &_session->sendOverlapped, nullptr);
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

bool LanClient::Disconnect()
{
	closesocket(_session->sock);

	return true;
}

bool LanClient::SendPacket(Packet& packet)
{
	_session->sendQ.Enqueue(packet.GetBufferPtr(), packet.GetDataSize());

	SendPost();

	return false;
}

unsigned int WINAPI LanClient::WorkerThread(LPVOID lpParam)
{
	LanClient* client = (LanClient*)lpParam;

	int retval;

	while (1)
	{
		// �񵿱� ����� �Ϸ� ��ٸ���
		DWORD Transferred;
		Session* session = NULL;
		OVERLAPPED* overlapped;
		retval = GetQueuedCompletionStatus(client->_hcp, &Transferred,
			(PULONG_PTR)&session, &overlapped, INFINITE);

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

			while (1)
			{
				if (session->recvQ.GetUseSize() < HEADER_SIZE)
					break;

				PACKET_HEADER header;
				session->recvQ.Peek((char*)&header, HEADER_SIZE);
				if (session->recvQ.GetUseSize() < HEADER_SIZE + header.bySize)
					break;

				// �޽��� ����
				Packet packet;
				retval = session->recvQ.Dequeue(packet.GetBufferPtr(), HEADER_SIZE + header.bySize);
				packet.MoveWritePos(retval);

				// �޼��� ó���ϴ� �Լ�
				client->OnRecv(packet);
			}

			// Recv �Ŵ� �Լ�
			client->RecvPost();
		}
		// 4. Send IO
		else if (overlapped == &session->sendOverlapped)
		{
			session->sendQ.MoveFront(Transferred);

			session->WSASend = 0;
			InterlockedDecrement(&session->IOCount);

			// Dispatch Message
			if (session->sendQ.GetUseSize() > 0)
				client->SendPost();
		}

		// ���� �����۱����� ���� ���� ����.
		LeaveCriticalSection(&session->cs);

		if (session->IOCount == 0)
		{
			client->Disconnect();
		}
	}

	return 0;
}