#pragma once
#include "Packet.h"
#include <WS2tcpip.h>
#include <process.h>

class LanServer
{
	using SessionID = unsigned __int64;

public:
	LanServer();
	virtual ~LanServer();

	// �� ��, �����ϸ� ���� �� � �� ������?
	// ���ε� ����(IP/Port), ��Ŀ������ ��, ���̱�, �ִ������� ��
	bool	Start(WCHAR IP[], int Port, int WorkerThreadCount, bool Nagle, int MaxUserCount);
	// ���� ����. ���� ������ ��� �ȴ�.
	void	Stop();
	int		GetSessionCount();

	bool	Disconnect(SessionID sessionID);
	bool	SendPacket(SessionID sessionID, Packet* packet);

	virtual bool	OnConnectionRequest(WCHAR IP[], int Port) = 0;
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

protected:
	static unsigned int WINAPI	AcceptThread(LPVOID lpParam);

	static unsigned int WINAPI	WorkerThread(LPVOID lpParam);

private:
	SOCKET	listenSock;
	HANDLE* WorkerThreads;

	int		AcceptTPS;
	int		RecvMessageTPS;
	int		SendMessageTPS;
};