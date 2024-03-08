#pragma once
#include "Session.h"
#include <process.h>
#include <unordered_map>
#include "Packet.h"
#include "Log.h"

class LanServer
{
	using SessionID = unsigned __int64;

public:
	LanServer();
	virtual ~LanServer();

	// 이 때, 설정하면 좋은 게 어떤 게 있을까?
	// 바인딩 정보(IP/Port), 워커스레드 수, 네이글, 최대접속자 수
	bool	Start(WCHAR IP[], int Port, int WorkerThreadCount, bool Nagle, int MaxUserCount);
	// 서버 종료. 굳이 재사용은 없어도 된다.
	void	Stop();
	int		GetSessionCount();

	bool	Disconnect(SessionID sessionID);
	bool	SendPacket(SessionID sessionID, Packet* packet);

	virtual bool	OnConnectionRequest(WCHAR IP[], int Port) = 0;
	// return false; 시 클라이언트 거부
	// return true; 시 접속 허용
	
	// 접속 완료 후 호출
	virtual void	OnAccept(SessionID) = 0;
	// 접속 끊음 후 호출
	virtual void	OnRelease() = 0;
	// 패킷 수신 완료
	virtual void	OnRecv() = 0;
	virtual void	OnError(int errorCode, WCHAR* text) = 0;

	int		GetAcceptTPS();
	int		GetRecvMessageTPS();
	int		GetSendMessageTPS();

protected:
	static unsigned int WINAPI	AcceptThread(LPVOID lpParam);

	static unsigned int WINAPI	WorkerThread(LPVOID lpParam);

	static unsigned int WINAPI	CalTPSThread(LPVOID lpParam);

private:
	HANDLE* _WorkerThreads;
	
	static SOCKET									listenSock;
	static CRITICAL_SECTION							mapCs;
	static std::unordered_map<__int64, Session*>	SessionMap;
	static unsigned __int64							UniqueID;

	// 모니터링용
	static int		AcceptTPS;
	static int		RecvMessageTPS;
	static int		SendMessageTPS;
};