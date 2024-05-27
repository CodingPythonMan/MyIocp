#include "GameClient.h"

bool GameClient::OnConnectionRequest(WCHAR IP[], int Port)
{
	return false;
}

void GameClient::OnRecv(Packet& packet)
{
	// ÄÁÅÙÃ÷ ÄÚµå


	//SendPacket(session->sessionID, packet);
}

void GameClient::OnError(int errorCode, WCHAR* text)
{

}