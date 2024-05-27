#include "GameClient.h"

int main()
{
	WCHAR IP[16];
	int Port;
	printf("접속할 IP 주소를 입력하세요 : ");
	wscanf_s(L"%s", IP, 16);
	IP[wcslen(IP)] = '\0';

	printf("접속할 포트를 입력하세요 : ");
	scanf_s("%d", &Port);
	
	GameClient gameClient;
	// 스레드는 1
	gameClient.Start(IP, Port, 1);

	Sleep(INFINITE);

	return 0;
}