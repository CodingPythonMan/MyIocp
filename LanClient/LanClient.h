#pragma once
#include "Session.h"
#include <process.h>
#include <unordered_map>
#include "Packet.h"
#include "Log.h"
#include "Protocol.h"

class LanClient
{
	using SessionID = unsigned __int64;

public:
	LanClient();
	virtual ~LanClient();

	// �� ��, �����ϸ� ���� �� � �� ������?
	// ���ε� ����(IP/Port), ��Ŀ������ ��
	bool	Start(WCHAR IP[], int Port, int WorkerThreadCount);
	// ���� ����. ���� ������ ��� �ȴ�.
	void	Stop();

	void	RecvPost();
	void	SendPost();

	bool	Disconnect();
	bool	SendPacket(Packet& packet);

	// ��Ŷ ���� �Ϸ�
	virtual void	OnRecv(Packet& packet) = 0;
	virtual void	OnError(int errorCode, WCHAR* text) = 0;

protected:
	static unsigned int WINAPI	WorkerThread(LPVOID lpParam);

protected:
	Session*								_session;

	// IOCP
	HANDLE									_hcp;
	HANDLE*									_WorkerThreads;
};