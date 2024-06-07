#include "MiniDump.h"
#include <windows.h>
#include <filesystem>
#include <minidumpapiset.h>

//#pragma comment(lib, "BugTrapU-x64.lib")

void MiniDump::InitExceptionHandler(bool autoRestart)
{
	// 현재 실행파일 이름
	WCHAR appName[MAX_PATH] = {};
	GetModuleFileNameW(nullptr, appName, MAX_PATH);

	std::filesystem::path fullPath(appName);

	DWORD dumpType = MiniDumpWithFullMemory // 모든 읽을 수 있는 프로세스 내 페이지를 덤프에 포함
		| MiniDumpWithHandleData // 핸들 테이블에 있던 모든 핸들 데이터 포함
		| MiniDumpWithThreadInfo // 스레드 시작, 유연 주소 포함
		| MiniDumpWithProcessThreadData // TEB(Thread Environment Blocks), PEB 포함
		| MiniDumpWithFullMemoryInfo // 가상주소 정보 레이아웃
		| MiniDumpWithUnloadedModules // OS에서 지원되는 최근 빠진 모듈들
		| MiniDumpWithFullAuxiliaryState // 덤프 안 그들의 상태를 담은 Aux 데이터 
		| MiniDumpIgnoreInaccessibleMemory // 메모리 읽기 실패 제외
		| MiniDumpWithTokenInformation // 관계있는 보안 토큰 포함
		;

	// BugTrap 등록

}