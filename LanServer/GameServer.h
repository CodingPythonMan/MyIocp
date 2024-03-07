#pragma once
#include "LanServer.h"

class GameServer : public LanServer
{
	// Inherited via LanServer
	virtual bool OnConnectionRequest(WCHAR IP[], int Port) override;
	virtual void OnAccept() override;
	virtual void OnRelease() override;
	virtual void OnRecv() override;
	virtual void OnError(int errorCode, WCHAR* text) override;
};

