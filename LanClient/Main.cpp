#include <iostream>
#include <windows.h>
#include "LanClient.h"

int main()
{
	printf("������ IP �ּҸ� �Է��ϼ��� : ");
	WCHAR IP[16];
	printf("������ ��Ʈ�� �Է��ϼ��� : ");
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