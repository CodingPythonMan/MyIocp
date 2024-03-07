#include "GameServer.h"

bool GameServer::OnConnectionRequest(WCHAR IP[], int Port)
{
	return false;
}

void GameServer::OnAccept()
{
}

void GameServer::OnRelease()
{
}

void GameServer::OnRecv()
{
}

void GameServer::OnError(int errorCode, WCHAR* text)
{
}
