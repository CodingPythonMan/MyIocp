#pragma once
#include "Packet.h"
#include <windows.h>

class LanServer
{
	using SessionID = unsigned __int64;

public:
	bool	Start();
	void	Stop();
	int		GetSessionCount();

	bool	Disconnect(SessionID sessionID);
	bool	SendPacket(SessionID sessionID, Packet* packet);

	virtual bool	OnConnectionRequest(WCHAR IP[], short Port) = 0;
	// return false; 시 클라이언트 거부
	// return true; 시 접속 허용
	
	// 접속 완료 후 호출
	virtual void	OnAccept() = 0;
	// 접속 끊음 후 호출
	virtual void	OnRelease() = 0;
	// 패킷 수신 완료
	virtual void	OnRecv() = 0;
	virtual void	OnError(int errorCode, WCHAR* text) = 0;

	int		GetAcceptTPS();
	int		GetRecvMessageTPS();
	int		GetSendMessageTPS();

private:
	int		AcceptTPS;
	int		RecvMessageTPS;
	int		SendMessageTPS;
};