#include <iostream>
#include <windows.h>
#include "LanClient.h"

int main()
{
	printf("접속할 IP 주소를 입력하세요 : ");
	WCHAR IP[16];
	printf("접속할 포트를 입력하세요 : ");
	wscanf_s(L"%s", IP, 16);
	IP[wcslen(IP)] = '\0';

	if (SetClientSocket())
		return 0;

	if (ConnectClientSocket(IP))
		return 0;

	if (SelectLoop())
		return 0;

	if (EndSocket())
		return 0;

	return 0;
}