#include "Log.h"
#include <time.h>

// extern
int gLogLevel;			// 출력 저장 대상의 로그 레벨
WCHAR gLogBuff[1024];	// 로그 저장 시 필요한 임시 버퍼

// global
char FileName[80];

void LogFileInit()
{
	tm t;
	time_t timer = time(nullptr); // 현재 시각을 초 단위로
	localtime_s(&t, &timer); // 초 단위 시간 분리 후, 구조체

	gLogLevel = LOG_LEVEL_DEBUG;

	sprintf_s(FileName, "%s_%4d_%02d_%02d.txt", "Log", 1900 + t.tm_year, t.tm_mon + 1, t.tm_mday);
}

void Log(WCHAR* String, int LogLevel)
{
	tm t;
	time_t timer = time(nullptr); // 현재 시각을 초 단위로
	localtime_s(&t, &timer); // 초 단위 시간 분리 후, 구조체

	WCHAR timeString[1104];
	swprintf_s(timeString, 1104, L"[%4d-%02d-%02d %02d:%02d:%02d] %s\n", 1900 + t.tm_year, t.tm_mon + 1, t.tm_mday,
		t.tm_hour, t.tm_min, t.tm_sec, String);
	wprintf(L"%s", timeString);

	FILE* file;
	fopen_s(&file, FileName, "ab+");
	if (file == nullptr)
		return;

	WCHAR bom = 0xFEFF;
	fwrite(&bom, 2, 1, file);
	fwrite(timeString, 2, wcslen(timeString), file);

	fclose(file);
}