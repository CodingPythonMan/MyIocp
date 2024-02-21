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
};