// Vortex64-Windows: Windows implementation of SyncEvent.h using native events
#include "../SyncEvent.h"
#include <windows.h>

SyncEvent::SyncEvent(bool bManualReset) :
    m_Event(CreateEvent(nullptr, bManualReset, false, nullptr)),
    m_cond(nullptr),
    m_signalled(false),
    m_manualReset(bManualReset)
{
}

SyncEvent::~SyncEvent()
{
    CloseHandle((HANDLE)m_Event);
}

void SyncEvent::Trigger()
{
    SetEvent((HANDLE)m_Event);
}

bool SyncEvent::IsTriggered(int32_t iWaitTime) const
{
    return WaitForSingleObject((HANDLE)m_Event, (DWORD)iWaitTime) == WAIT_OBJECT_0;
}

void SyncEvent::Reset()
{
    ResetEvent((HANDLE)m_Event);
}

void * SyncEvent::GetHandle()
{
    return m_Event;
}
