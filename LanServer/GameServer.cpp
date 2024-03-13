#include "GameServer.h"

bool GameServer::OnConnectionRequest(WCHAR IP[], int Port)
{
	return false;
}

void GameServer::OnAccept(SessionID sessionID)
{

}

void GameServer::OnRelease()
{
}

void GameServer::OnRecv(SessionID sessionID, Packet& packet)
{
	// ������ �ڵ�
	Session* session = FindSession(sessionID);

	unsigned short len;
	unsigned __int64 detail;

	packet >> len;
	packet >> detail;

	InterlockedIncrement((long*)&_RecvMessageTPS);

	// �ٽ� ������
	// SendPacket
	Packet* sendPacket = new Packet(len + sizeof(detail));
	*sendPacket << len;
	*sendPacket << detail;
	SendPacket(session->sessionID, sendPacket);
}

void GameServer::OnError(int errorCode, WCHAR* text)
{
}
