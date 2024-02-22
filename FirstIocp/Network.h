#pragma once
#include "Session.h"

#define SERVERPORT 9000

void NetworkInit();

void NetworkIO();

unsigned int WINAPI AcceptThread(LPVOID lpParam);
unsigned int WINAPI WorkerThread(LPVOID lpParam);