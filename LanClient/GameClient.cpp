#include "GameClient.h"

bool GameClient::OnConnectionRequest(WCHAR IP[], int Port)
{
	return false;
}

void GameClient::OnRecv(Packet& packet)
{
	// 컨텐츠 코드
	PACKET_HEADER header;
	packet.GetData((char*)&header, sizeof(PACKET_HEADER));

	// Code 가 정확하지 않을 때, return
	if (header.byCode != NETWORK_CODE)
	{
		return;
	}

	switch (header.byType)
	{
	case REP_LOGIN:
	{
		unsigned __int64 sessionID;
		packet >> sessionID;
		printf("로그인 완료! SessionID : %lld\n", sessionID);
		_session->sessionID = sessionID;
		break;
	}
	case REP_MOVE_START:
	{	
		
		
		break;
	}
	case REP_MOVE_STOP:
	{
		
		
		break;
	}
	}
}

void GameClient::OnError(int errorCode, WCHAR* text)
{

}