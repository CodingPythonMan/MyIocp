#pragma once
#pragma comment(lib, "ws2_32")
#include <WS2tcpip.h>
#include "RingBuffer.h"

struct Session 
{
	OVERLAPPED recvOverlapped;
	OVERLAPPED sendOverlapped;
	SOCKET sock;
	RingBuffer sendQ;
	RingBuffer recvQ;

	Session(SOCKET clientSock)
	{
		memset(&recvOverlapped, 0, sizeof(recvOverlapped));
		memset(&sendOverlapped, 0, sizeof(sendOverlapped));
		sock = clientSock;
	}
};