#pragma once
#include <windows.h>
#include <stdio.h>

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_SYSTEM 2

void LogFileInit();

void Log(WCHAR* String, int LogLevel);

extern int gLogLevel;			// 출력 저장 대상의 로그 레벨
extern WCHAR gLogBuff[1024];	// 로그 저장 시 필요한 임시 버퍼

#define _LOG(LogLevel, fmt, ...)					\
do{													\
	if (gLogLevel <= LogLevel)						\
	{												\
		wsprintf(gLogBuff, fmt, ##__VA_ARGS__);		\
		Log(gLogBuff, LogLevel);					\
	}												\
}while(0)