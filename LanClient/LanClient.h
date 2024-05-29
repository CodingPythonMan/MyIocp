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

	// 이 때, 설정하면 좋은 게 어떤 게 있을까?
	// 바인딩 정보(IP/Port), 워커스레드 수
	bool	Start(WCHAR IP[], int Port, int WorkerThreadCount);
	// 서버 종료. 굳이 재사용은 없어도 된다.
	void	Stop();

	void	RecvPost();
	void	SendPost();

	bool	Disconnect();
	bool	SendPacket(Packet& packet);

	// 패킷 수신 완료
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