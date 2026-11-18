#include "CrashHandler.h"

LONG __stdcall ExportDump(EXCEPTION_POINTERS* exception)
{

	SetUnhandledExceptionFilter(ExportDump);

	SYSTEMTIME time;

	GetLocalTime(&time);

	wchar_t filePath[MAX_PATH] = { 0 };

	CreateDirectory(L"./Dumps", nullptr);

	StringCchPrintf(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);

	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);


	if (dumpFileHandle != INVALID_HANDLE_VALUE) {

		DWORD processId = GetCurrentProcessId();

		DWORD threadId = GetCurrentThreadId();

		MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };

		minidumpInformation.ThreadId = threadId;

		minidumpInformation.ExceptionPointers = exception;

		minidumpInformation.ClientPointers = TRUE;


		MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

		CloseHandle(dumpFileHandle);
	}

	return EXCEPTION_EXECUTE_HANDLER;

}
