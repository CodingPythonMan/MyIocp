#pragma once
#include "LanClient.h"

class GameClient : public LanClient
{
	using SessionID = unsigned __int64;
public:
	// Inherited via LanServer
	virtual bool OnConnectionRequest(WCHAR IP[], int Port) override;
	virtual void OnAccept(SessionID sessionID) override;
	virtual void OnRelease() override;
	virtual void OnRecv(SessionID sessionID, Packet& packet) override;
	virtual void OnError(int errorCode, WCHAR* text) override;
};