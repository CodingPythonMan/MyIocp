#pragma once
#include "LanServer.h"
#include "Player.h"
#include <map>

class GameServer : public LanServer
{
	using SessionID = unsigned __int64;
public:
	GameServer();
	virtual ~GameServer();

	// Inherited via LanServer
	virtual bool OnConnectionRequest(WCHAR IP[], int Port) override;
	virtual void OnAccept(SessionID sessionID) override;
	virtual void OnRelease(SessionID sessionID) override;
	virtual void OnRecv(SessionID sessionID, Packet& packet) override;
	virtual void OnError(int errorCode, WCHAR* text) override;

	// BroadCast
	bool SendBroadcast(Packet& packet);
	
	Player* FindPlayer(SessionID sessionID);

private:
	std::map<SessionID, Player*>	_playerMap;

	CRITICAL_SECTION				_playerMapLock;
};