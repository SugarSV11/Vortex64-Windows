// Vortex64-Windows: Windows implementation of CriticalSection.h (CRITICAL_SECTION is recursive)
#include "../CriticalSection.h"
#include <windows.h>

CriticalSection::CriticalSection()
{
    CRITICAL_SECTION * cs = new CRITICAL_SECTION;
    InitializeCriticalSection(cs);
    m_cs = cs;
}

CriticalSection::~CriticalSection(void)
{
    DeleteCriticalSection((CRITICAL_SECTION *)m_cs);
    delete (CRITICAL_SECTION *)m_cs;
}

void CriticalSection::enter(void)
{
    EnterCriticalSection((CRITICAL_SECTION *)m_cs);
}

void CriticalSection::leave(void)
{
    LeaveCriticalSection((CRITICAL_SECTION *)m_cs);
}
