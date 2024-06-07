#include "MiniDump.h"
#include <windows.h>
#include <filesystem>
#include <minidumpapiset.h>

//#pragma comment(lib, "BugTrapU-x64.lib")

void MiniDump::InitExceptionHandler(bool autoRestart)
{
	// ���� �������� �̸�
	WCHAR appName[MAX_PATH] = {};
	GetModuleFileNameW(nullptr, appName, MAX_PATH);

	std::filesystem::path fullPath(appName);

	DWORD dumpType = MiniDumpWithFullMemory // ��� ���� �� �ִ� ���μ��� �� �������� ������ ����
		| MiniDumpWithHandleData // �ڵ� ���̺� �ִ� ��� �ڵ� ������ ����
		| MiniDumpWithThreadInfo // ������ ����, ���� �ּ� ����
		| MiniDumpWithProcessThreadData // TEB(Thread Environment Blocks), PEB ����
		| MiniDumpWithFullMemoryInfo // �����ּ� ���� ���̾ƿ�
		| MiniDumpWithUnloadedModules // OS���� �����Ǵ� �ֱ� ���� ����
		| MiniDumpWithFullAuxiliaryState // ���� �� �׵��� ���¸� ���� Aux ������ 
		| MiniDumpIgnoreInaccessibleMemory // �޸� �б� ���� ����
		| MiniDumpWithTokenInformation // �����ִ� ���� ��ū ����
		;

	// BugTrap ���

}