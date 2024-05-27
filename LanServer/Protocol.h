#pragma once

#define HEADER_SIZE 3
#define NETWORK_CODE 0x89

struct PACKET_HEADER
{
	unsigned char	byCode;
	unsigned char	bySize;	
	unsigned char	byType;
};

// ����
#define QRY_LOGIN 1
// Session ID (8)
#define REP_LOGIN 2

// Session ID (8)  
#define QRY_MOVE 3
// Session ID (8) Direction (1)
#define REP_MOVE 4