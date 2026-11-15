// Vortex64-Windows: Windows implementation of MemoryManagement.h (replaces the POSIX mmap version).
#include "../MemoryManagement.h"
#include <windows.h>

static bool TranslateFromMemProtect(MEM_PROTECTION p, DWORD & os)
{
    switch (p)
    {
    case MEM_NOACCESS: os = PAGE_NOACCESS; return true;
    case MEM_READONLY: os = PAGE_READONLY; return true;
    case MEM_READWRITE: os = PAGE_READWRITE; return true;
    case MEM_EXECUTE_READWRITE: os = PAGE_EXECUTE_READWRITE; return true;
    }
    return false;
}

void * AllocateAddressSpace(size_t size, void * base_address)
{
    return VirtualAlloc(base_address, size, MEM_RESERVE, PAGE_NOACCESS);
}

bool FreeAddressSpace(void * addr, size_t)
{
    if (addr == nullptr)
    {
        return true;
    }
    return VirtualFree(addr, 0, MEM_RELEASE) != 0;
}

void * CommitMemory(void * addr, size_t size, MEM_PROTECTION memProtection)
{
    DWORD os;
    if (!TranslateFromMemProtect(memProtection, os))
    {
        return nullptr;
    }
    return VirtualAlloc(addr, size, MEM_COMMIT, os);
}

bool DecommitMemory(void * addr, size_t size)
{
    return VirtualFree(addr, size, MEM_DECOMMIT) != 0;
}

bool ProtectMemory(void * addr, size_t size, MEM_PROTECTION memProtection, MEM_PROTECTION * OldProtect)
{
    DWORD os, old = 0;
    if (!TranslateFromMemProtect(memProtection, os))
    {
        return false;
    }
    if (!VirtualProtect(addr, size, os, &old))
    {
        return false;
    }
    if (OldProtect != nullptr)
    {
        switch (old)
        {
        case PAGE_READONLY: *OldProtect = MEM_READONLY; break;
        case PAGE_READWRITE: *OldProtect = MEM_READWRITE; break;
        case PAGE_EXECUTE_READWRITE: *OldProtect = MEM_EXECUTE_READWRITE; break;
        default: *OldProtect = MEM_NOACCESS; break;
        }
    }
    return true;
}
