#pragma once
#include <Windows.h>
#include<DbgHelp.h>
#include<strsafe.h>

#pragma comment(lib, "Dbghelp.lib")


LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);
