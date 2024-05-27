#pragma once
#include "LanClient.h"

class GameClient : public LanClient
{
	using SessionID = unsigned __int64;
public:
	// Inherited via LanServer
	virtual bool OnConnectionRequest(WCHAR IP[], int Port) override;
	virtual void OnRecv(Packet& packet) override;
	virtual void OnError(int errorCode, WCHAR* text) override;
};