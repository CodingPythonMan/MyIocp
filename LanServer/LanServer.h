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
	// return false; �� Ŭ���̾�Ʈ �ź�
	// return true; �� ���� ���
	
	// ���� �Ϸ� �� ȣ��
	virtual void	OnAccept() = 0;
	// ���� ���� �� ȣ��
	virtual void	OnRelease() = 0;
	// ��Ŷ ���� �Ϸ�
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