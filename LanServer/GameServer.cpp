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
	// 컨텐츠 코드
	Session* session = FindSession(sessionID);
	
	PACKET_HEADER header;
	packet.GetData((char*)&header, sizeof(PACKET_HEADER));

	// Code 가 정확하지 않을 때, return
	if (header.byCode != NETWORK_CODE)
	{
		return;
	}

	switch (header.byType)
	{
	case QRY_LOGIN:
	{
		packet.Clear();
		header.byType = REP_LOGIN;
		packet.PutData((char*)&header, HEADER_SIZE);
		packet << sessionID;
		SendPacket(sessionID, packet);
		break;
	}
	case QRY_MOVE:
	{


		packet.Clear();
		header.byType = REP_MOVE;
		packet.PutData((char*)&header, HEADER_SIZE);
		packet << sessionID;
		//packet << 
		SendPacket(sessionID, packet);
		break;
	}
	}

	InterlockedIncrement((long*)&_RecvMessageTPS);
	//SendPacket(session->sessionID, packet);
}

void GameServer::OnError(int errorCode, WCHAR* text)
{
}

bool GameServer::SendBroadcast(Packet& packet)
{

	return false;
}