#pragma once
#pragma comment(lib, "ws2_32")
#include <WS2tcpip.h>
#include "RingBuffer.h"

struct Session 
{
	__int64 sessionID;
	long IOCount;
	int send;
	int sendPost;
	int sendBytes;
	int sendPostBytes;
	OVERLAPPED recvOverlapped;
	OVERLAPPED sendOverlapped;
	SOCKET sock;
	RingBuffer sendQ;
	RingBuffer recvQ;
	CRITICAL_SECTION cs;

	Session(__int64 id, SOCKET clientSock)
	{
		sessionID = id;
		IOCount = 0;
		send = 0;
		sendPost = 0;
		sendBytes = 0;
		sendPostBytes = 0;
		memset(&recvOverlapped, 0, sizeof(recvOverlapped));
		memset(&sendOverlapped, 0, sizeof(sendOverlapped));
		sock = clientSock;
		InitializeCriticalSection(&cs);
	}
};