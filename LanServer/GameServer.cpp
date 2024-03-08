#include "GameServer.h"

bool GameServer::OnConnectionRequest(WCHAR IP[], int Port)
{
	return false;
}

void GameServer::OnAccept(SessionID sessionID)
{

}

void GameServer::OnRelease()
{
}

void GameServer::OnRecv(SessionID sessionID, Packet& packet)
{
	// 컨텐츠 코드
	Session* session = _SessionMap[sessionID];
	
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
	SendPacket(session->sessionID, packet);
}

void GameServer::OnError(int errorCode, WCHAR* text)
{
}
