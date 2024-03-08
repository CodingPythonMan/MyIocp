#pragma once
#include <windows.h>
#include <stdio.h>

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_SYSTEM 2

void LogFileInit();

void Log(WCHAR* String, int LogLevel);

extern int gLogLevel;			// ��� ���� ����� �α� ����
extern WCHAR gLogBuff[1024];	// �α� ���� �� �ʿ��� �ӽ� ����

#define _LOG(LogLevel, fmt, ...)					\
do{													\
	if (gLogLevel <= LogLevel)						\
	{												\
		wsprintf(gLogBuff, fmt, ##__VA_ARGS__);		\
		Log(gLogBuff, LogLevel);					\
	}												\
}while(0)