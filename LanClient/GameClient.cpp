#include "GameClient.h"

bool GameClient::OnConnectionRequest(WCHAR IP[], int Port)
{
	return false;
}

void GameClient::OnAccept(SessionID sessionID)
{

}

void GameClient::OnRelease()
{
}

void GameClient::OnRecv(SessionID sessionID, Packet& packet)
{
	// ������ �ڵ�
	Session* session = FindSession(sessionID);

	// Ŭ���̾�Ʈ ���� ���
	SOCKADDR_IN clientAddr;
	WCHAR IP[16];

	int addrLen = sizeof(clientAddr);
	getpeername(session->sock, (SOCKADDR*)&clientAddr, &addrLen);

	InetNtop(AF_INET, &(clientAddr.sin_addr), IP, 16);
	//wprintf(L"[TCP/%s:%d] ", IP, ntohs(clientAddr.sin_port));

	//printf("%s\n", packet.GetBufferPtr());

	// �ٽ� ������
	// SendPacket
	InterlockedIncrement((long*)&_RecvMessageTPS);
	SendPacket(session->sessionID, packet);
}

void GameClient::OnError(int errorCode, WCHAR* text)
{

}