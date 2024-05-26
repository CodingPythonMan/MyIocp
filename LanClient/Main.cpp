#include "GameClient.h"

int main()
{
	WCHAR IP[16];
	printf("접속할 IP 주소를 입력하세요 : ");
	wscanf_s(L"%s", IP, 16);
	
	printf("접속할 포트를 입력하세요 : ");
	wscanf_s(L"%s", IP, 16);
	IP[wcslen(IP)] = '\0';

	return 0;
}