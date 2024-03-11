#include "GameServer.h"
#include "Parser.h"

int main()
{
	GameServer gameServer;
	
	WCHAR* IP;
	int Port;
	int WorkerThread;
	int Nagle;
	int MaxUserCount;

	Parser* parser = Parser::GetInstance();
	parser->LoadFile("GameConfig.ini");
	parser->GetValueWStr("IP", &IP);
	parser->GetValueInt("Port", &Port);
	parser->GetValueInt("WorkerThread", &WorkerThread);
	parser->GetValueInt("Nagle", &Nagle);
	parser->GetValueInt("MaxUserCount", &MaxUserCount);

	gameServer.Start(IP, Port, WorkerThread, Nagle, MaxUserCount);

	Sleep(INFINITE);

	return 0;
}