#include "GameClient.h"

int main()
{
	WCHAR IP[16];
	printf("������ IP �ּҸ� �Է��ϼ��� : ");
	wscanf_s(L"%s", IP, 16);
	
	printf("������ ��Ʈ�� �Է��ϼ��� : ");
	wscanf_s(L"%s", IP, 16);
	IP[wcslen(IP)] = '\0';

	return 0;
}