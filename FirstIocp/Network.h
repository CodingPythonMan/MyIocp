#pragma once
#include "Session.h"
#include "Packet.h"

#define SERVERPORT 6000

void NetworkInit();

void NetworkIO();

unsigned int WINAPI AcceptThread(LPVOID lpParam);
unsigned int WINAPI WorkerThread(LPVOID lpParam);

void OnAccept(__int64 sessionID);
void OnRecv(Session* session, Packet& packet);

void RecvPost(Session* session);
void SendPost(Session* session);

void SendPacket(__int64 sessionID, Packet& packet);