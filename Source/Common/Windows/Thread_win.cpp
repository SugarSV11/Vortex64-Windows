// Vortex64-Windows: Windows implementation of Thread.h
#include "../Thread.h"
#include "../Trace.h"
#include "../TraceModulesCommon.h"
#include "../Platform.h"

CThread::CThread(CTHREAD_START_ROUTINE lpStartAddress) :
    m_StartAddress(lpStartAddress),
    m_lpThreadParameter(nullptr),
    m_thread(nullptr),
    m_threadID(0),
    m_running(false)
{
}

CThread::~CThread()
{
    if (CThread::GetCurrentThreadId() == m_threadID)
    {
        WriteTrace(TraceThread, TraceError, "Deleting from thread!");
    }
    if (CThread::GetCurrentThreadId() != m_threadID && isRunning())
    {
        Terminate();
    }
    Wait();
}

bool CThread::Start(void * lpThreadParameter)
{
    m_lpThreadParameter = lpThreadParameter;
    if (m_thread != nullptr)
    {
        if (isRunning())
        {
            WriteTrace(TraceThread, TraceError, "Thread is already running");
            return false;
        }
        Wait();
    }
    m_running = true;
    DWORD id = 0;
    HANDLE h = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ThreadWrapper, this, 0, &id);
    if (h == nullptr)
    {
        m_running = false;
        return false;
    }
    m_thread = h;
    m_threadID = id;
    return true;
}

void * CThread::ThreadWrapper(CThread * _this)
{
    _this->m_threadID = CThread::GetCurrentThreadId();
    _this->m_running = true;
    try
    {
        _this->m_StartAddress(_this->m_lpThreadParameter);
    }
    catch (...)
    {
    }
    _this->m_running = false;
    return nullptr;
}

bool CThread::isRunning(void) const
{
    return m_thread != nullptr && m_running;
}

void CThread::Terminate(void)
{
    if (isRunning() && CThread::GetCurrentThreadId() != m_threadID)
    {
        // Same behaviour as upstream Project64: forced termination (no unwinding)
        TerminateThread((HANDLE)m_thread, 0);
        m_running = false;
        Wait();
    }
}

void CThread::Wait(void)
{
    if (m_thread == nullptr || CThread::GetCurrentThreadId() == m_threadID)
    {
        return;
    }
    WaitForSingleObject((HANDLE)m_thread, INFINITE);
    CloseHandle((HANDLE)m_thread);
    m_thread = nullptr;
    m_threadID = 0;
    m_running = false;
}

uint32_t CThread::GetCurrentThreadId(void)
{
    return ::GetCurrentThreadId();
}
