#include "GameClient.h"

int main()
{
	WCHAR IP[16];
	int Port;
	printf("������ IP �ּҸ� �Է��ϼ��� : ");
	wscanf_s(L"%s", IP, 16);
	IP[wcslen(IP)] = '\0';

	printf("������ ��Ʈ�� �Է��ϼ��� : ");
	scanf_s("%d", &Port);
	
	GameClient gameClient;
	// ������� 1
	gameClient.Start(IP, Port, 1);

	Sleep(INFINITE);

	return 0;
}