#include "GameServer.h"

GameServer::GameServer()
{
	InitializeCriticalSection(&_playerMapLock);
}

GameServer::~GameServer()
{
	DeleteCriticalSection(&_playerMapLock);
}

bool GameServer::OnConnectionRequest(WCHAR IP[], int Port)
{
	return false;
}

void GameServer::OnAccept(SessionID sessionID)
{
	Player* player = new Player();

	EnterCriticalSection(&_playerMapLock);
	_playerMap.insert({ sessionID, player });
	LeaveCriticalSection(&_playerMapLock);
}

void GameServer::OnRelease(SessionID sessionID)
{
	EnterCriticalSection(&_playerMapLock);
	Player* player = FindPlayer(sessionID);
	delete player;
	_playerMap.erase(sessionID);
	LeaveCriticalSection(&_playerMapLock);
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
		header.bySize = 8;
		packet.PutData((char*)&header, HEADER_SIZE);
		packet << sessionID;
		SendPacket(sessionID, packet);
		break;
	}
	case QRY_MOVE_START:
	{
		Player* player = FindPlayer(sessionID);
		packet.Clear();
		header.byType = REP_MOVE_START;
		packet.PutData((char*)&header, HEADER_SIZE);
		packet << player->X;
		packet << player->Y;
		SendPacket(sessionID, packet);
		break;
	}
	case QRY_MOVE_STOP:
	{
		Player* player = FindPlayer(sessionID);
		packet.Clear();
		header.byType = REP_MOVE_STOP;
		packet.PutData((char*)&header, HEADER_SIZE);
		packet << player->X;
		packet << player->Y;
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

Player* GameServer::FindPlayer(SessionID sessionID)
{
	return _playerMap[sessionID];
}