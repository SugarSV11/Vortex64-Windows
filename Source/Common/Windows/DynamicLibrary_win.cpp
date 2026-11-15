// Vortex64-Windows: Windows implementation of DynamicLibrary.h
#include "../DynamicLibrary.h"
#include <windows.h>
#include <string>

DynLibHandle DynamicLibraryOpen(const char * LibraryPath, bool)
{
    if (LibraryPath == nullptr)
    {
        return nullptr;
    }
    int n = MultiByteToWideChar(CP_UTF8, 0, LibraryPath, -1, nullptr, 0);
    if (n <= 0)
    {
        return nullptr;
    }
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, LibraryPath, -1, &w[0], n);
    return (DynLibHandle)LoadLibraryW(w.c_str());
}

void DynamicLibraryClose(DynLibHandle Lib)
{
    if (Lib != nullptr)
    {
        FreeLibrary((HMODULE)Lib);
    }
}

void * DynamicLibraryGetProc(DynLibHandle Lib, const char * ProcedureName)
{
    if (Lib == nullptr || ProcedureName == nullptr)
    {
        return nullptr;
    }
    return (void *)GetProcAddress((HMODULE)Lib, ProcedureName);
}
