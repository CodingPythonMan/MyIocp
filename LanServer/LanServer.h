#pragma once
#include "Session.h"
#include <process.h>
#include <unordered_map>
#include "Packet.h"
#include "Log.h"
#include "Protocol.h"
#include <stack>

#define MAX_UNIQUE_ID 275,000,000,000,000

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

	void	RecvPost(SessionID sessionID);
	void	SendPost(SessionID sessionID);

	bool	Disconnect(SessionID sessionID);
	bool	SendPacket(SessionID sessionID, Packet& packet);

	virtual bool	OnConnectionRequest(WCHAR IP[], int Port) = 0;
	// return false; �� Ŭ���̾�Ʈ �ź�
	// return true; �� ���� ���
	
	// ���� �Ϸ� �� ȣ��
	virtual void	OnAccept(SessionID sessionID) = 0;
	// ���� ���� �� ȣ��
	virtual void	OnRelease() = 0;
	// ��Ŷ ���� �Ϸ�
	virtual void	OnRecv(SessionID sessionID, Packet& packet) = 0;
	virtual void	OnError(int errorCode, WCHAR* text) = 0;

	int			GetAcceptTPS();
	int			GetRecvMessageTPS();
	int			GetSendMessageTPS();

	// Session �����ϹǷ� ���� �˻� ����
	Session*		FindSession(SessionID sessionID);
	unsigned short	ConvertIndex(SessionID sessionID);

protected:
	static unsigned int WINAPI	AcceptThread(LPVOID lpParam);

	static unsigned int WINAPI	WorkerThread(LPVOID lpParam);

	static unsigned int WINAPI	CalTPSThread(LPVOID lpParam);

protected:
	SOCKET									_listenSock;

	// IOCP
	HANDLE									_hcp;
	HANDLE*									_WorkerThreads;
	
	// Resource
	Session**								_SessionArray;
	CRITICAL_SECTION						_SessionIndexLock;
	std::stack<unsigned short>				_SessionIndex;

	unsigned long							_SessionCount;
	unsigned __int64						_UniqueID;

	// ����͸���
	int										_AcceptTPS;
	int										_RecvMessageTPS;
	int										_SendMessageTPS;

	int										_TotalAccept;
};