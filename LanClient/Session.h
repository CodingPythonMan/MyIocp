#pragma once
#include <WS2tcpip.h>
#include "RingBuffer.h"

#pragma comment(lib, "ws2_32")

struct Session
{
	unsigned __int64 sessionID;
	long IOCount;
	OVERLAPPED recvOverlapped;
	OVERLAPPED sendOverlapped;
	SOCKET sock;
	RingBuffer sendQ;
	RingBuffer recvQ;
	unsigned long WSASend;
	CRITICAL_SECTION cs;

	Session()
	{
		sessionID = 0;
		IOCount = 0;
		memset(&recvOverlapped, 0, sizeof(recvOverlapped));
		memset(&sendOverlapped, 0, sizeof(sendOverlapped));
		sock = INVALID_SOCKET;
		WSASend = 0;
		InitializeCriticalSection(&cs);
	}

	Session(__int64 id, SOCKET clientSock)
	{
		sessionID = id;
		IOCount = 0;
		memset(&recvOverlapped, 0, sizeof(recvOverlapped));
		memset(&sendOverlapped, 0, sizeof(sendOverlapped));
		sock = clientSock;
		WSASend = 0;
		InitializeCriticalSection(&cs);
	}
};