
//cl.exe dll-extractor.cpp
//x86_64-w64-mingw32-g++ dll-extractor.cpp -o dll-extractor.exe

//dll-extractor.exe NAMRPROCESS


#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <stdio.h>
#include <tchar.h>

// Function to find PID based on process name
DWORD FindProcessId(const TCHAR* processName)
{
    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof(processInfo);

    HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (processesSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    Process32First(processesSnapshot, &processInfo);
    if (!_tcsicmp(processInfo.szExeFile, processName)) {
        CloseHandle(processesSnapshot);
        return processInfo.th32ProcessID;
    }

    while (Process32Next(processesSnapshot, &processInfo)) {
        if (!_tcsicmp(processInfo.szExeFile, processName)) {
            CloseHandle(processesSnapshot);
            return processInfo.th32ProcessID;
        }
    }

    CloseHandle(processesSnapshot);
    return 0;
}

// Function to display DLLs used by the process
void PrintProcessDLLs(DWORD processID)
{
    HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;
    unsigned int i;

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if (NULL == hProcess) {
        _tprintf(_T("Cannot open process (Error: %d)\n"), GetLastError());
        return;
    }

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            TCHAR szModName[MAX_PATH];
            if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
                _tprintf(_T("%s\n"), szModName);
            }
        }
    }

    CloseHandle(hProcess);
}

int _tmain(int argc, TCHAR* argv[])
{
    if (argc != 2) {
        _tprintf(_T("Usage: %s <process_name>\n"), argv[0]);
        return 1;
    }

    DWORD pid = FindProcessId(argv[1]);
    if (pid == 0) {
        _tprintf(_T("Process '%s' not found!\n"), argv[1]);
        return 1;
    }

    _tprintf(_T("DLLs used by process %s (PID: %d):\n"), argv[1], pid);
    PrintProcessDLLs(pid);

    return 0;
}

