#pragma once

#define HEADER_SIZE 3
#define NETWORK_CODE 0x89

struct PACKET_HEADER
{
	unsigned char	byCode;
	unsigned char	bySize;
	unsigned char	byType;
};

// ¾øÀ½
#define QRY_LOGIN 1
// Session ID (8)
#define REP_LOGIN 2

// Session ID (8) Direction (1)  
#define QRY_MOVE_START 3
// Session ID (8) X (4) Y (4)
#define REP_MOVE_START 4

// Session ID (8) Direction (1)
#define QRY_MOVE_STOP 4
// Session ID (8) X (4) Y (4)
#define REP_MOVE_STOP 5