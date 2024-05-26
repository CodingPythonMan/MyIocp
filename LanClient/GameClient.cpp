#include "GameClient.h"

bool GameClient::OnConnectionRequest(WCHAR IP[], int Port)
{
	return false;
}

void GameClient::OnAccept(SessionID sessionID)
{

}

void GameClient::OnRelease()
{
}

void GameClient::OnRecv(SessionID sessionID, Packet& packet)
{
	// 컨텐츠 코드
	Session* session = FindSession(sessionID);

	// 클라이언트 정보 얻기
	SOCKADDR_IN clientAddr;
	WCHAR IP[16];

	int addrLen = sizeof(clientAddr);
	getpeername(session->sock, (SOCKADDR*)&clientAddr, &addrLen);

	InetNtop(AF_INET, &(clientAddr.sin_addr), IP, 16);
	//wprintf(L"[TCP/%s:%d] ", IP, ntohs(clientAddr.sin_port));

	//printf("%s\n", packet.GetBufferPtr());

	// 다시 재전송
	// SendPacket
	InterlockedIncrement((long*)&_RecvMessageTPS);
	SendPacket(session->sessionID, packet);
}

void GameClient::OnError(int errorCode, WCHAR* text)
{

}