//cl.exe function-extractor.cpp
//x86_64-w64-mingw32-g++ function-extractor.cpp -o function-extractor.exe -ldbghelp

//function-extractor.exe PATHDLL


#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

void DumpDLLFunctions(LPCTSTR dllPath)
{
    HMODULE hModule = LoadLibraryEx(dllPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (hModule == NULL)
    {
        _tprintf(_T("Failed to load DLL (Error: %d)\n"), GetLastError());
        return;
    }

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + pDosHeader->e_lfanew);

    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hModule +
    pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

    DWORD* pFunctions = (DWORD*)((BYTE*)hModule + pExportDir->AddressOfFunctions);
    DWORD* pNames = (DWORD*)((BYTE*)hModule + pExportDir->AddressOfNames);
    WORD* pOrdinals = (WORD*)((BYTE*)hModule + pExportDir->AddressOfNameOrdinals);

    _tprintf(_T("Exported functions from %s:\n"), dllPath);
    _tprintf(_T("--------------------------------\n"));

    // Create array for sorting by ordinal
    WORD* sortedOrdinals = (WORD*)malloc(pExportDir->NumberOfNames * sizeof(WORD));
    for (DWORD i = 0; i < pExportDir->NumberOfNames; i++) {
        sortedOrdinals[i] = pOrdinals[i];
    }

    // Sort by ordinal
    for (DWORD i = 0; i < pExportDir->NumberOfNames - 1; i++) {
        for (DWORD j = i + 1; j < pExportDir->NumberOfNames; j++) {
            if (sortedOrdinals[i] > sortedOrdinals[j]) {
                WORD temp = sortedOrdinals[i];
                sortedOrdinals[i] = sortedOrdinals[j];
                sortedOrdinals[j] = temp;
            }
        }
    }

    // Display functions by ordinal
    for (DWORD i = 0; i < pExportDir->NumberOfNames; i++)
    {
        // Find function with current ordinal
        for (DWORD j = 0; j < pExportDir->NumberOfNames; j++)
        {
            if (pOrdinals[j] == sortedOrdinals[i])
            {
                LPCSTR pFunctionName = (LPCSTR)((BYTE*)hModule + pNames[j]);
                _tprintf(_T("%-40s @%d\n"), pFunctionName, pOrdinals[j] + 1); // +1 because ordinals start from 1
                break;
            }
        }
    }

    free(sortedOrdinals);
    FreeLibrary(hModule);
}

int _tmain(int argc, TCHAR* argv[])
{
    if (argc != 2)
    {
        _tprintf(_T("Usage: %s <dll_path>\n"), argv[0]);
        _tprintf(_T("Example: %s c:\\users\\download\\io.dll\n"), argv[0]);
        return 1;
    }

    SymSetOptions(SYMOPT_UNDNAME);
    if (!SymInitialize(GetCurrentProcess(), NULL, TRUE))
    {
        _tprintf(_T("SymInitialize failed (Error: %d)\n"), GetLastError());
    }

    DumpDLLFunctions(argv[1]);

    SymCleanup(GetCurrentProcess());
    return 0;
}

