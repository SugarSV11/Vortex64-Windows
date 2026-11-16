// Vortex64-Windows: hooks of the script engine on Windows (the real Win32 API is used for everything else).
#include "script_win_hooks.h"
#undef CreateProcessA // this file calls the real function

ScriptShimHooks g_ScriptShimHooks = {nullptr, nullptr};

static CRITICAL_SECTION g_JobLock;
static INIT_ONCE g_JobOnce = INIT_ONCE_STATIC_INIT;
static HANDLE g_Job = nullptr;

static BOOL CALLBACK InitLock(PINIT_ONCE, PVOID, PVOID *)
{
    InitializeCriticalSection(&g_JobLock);
    return TRUE;
}

// Must be called with g_JobLock held.
static HANDLE EnsureJob()
{
    if (g_Job == nullptr)
    {
        HANDLE Job = CreateJobObjectW(nullptr, nullptr);
        if (Job != nullptr)
        {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION Info = {};
            Info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(Job, JobObjectExtendedLimitInformation, &Info, sizeof(Info));
            g_Job = Job;
        }
    }
    return g_Job;
}

BOOL ScriptShimCreateProcessImpl(LPCSTR App, LPSTR Cmd, LPSECURITY_ATTRIBUTES PA, LPSECURITY_ATTRIBUTES TA, BOOL Inherit,
                                 DWORD Flags, LPVOID Env, LPCSTR Dir, LPSTARTUPINFOA SI, LPPROCESS_INFORMATION PI)
{
    InitOnceExecuteOnce(&g_JobOnce, InitLock, nullptr, nullptr);
    // Start suspended so the child cannot start grandchildren before it is in the job.
    bool WasSuspended = (Flags & CREATE_SUSPENDED) != 0;
    if (!CreateProcessA(App, Cmd, PA, TA, Inherit, Flags | CREATE_SUSPENDED, Env, Dir, SI, PI))
    {
        return FALSE;
    }
    EnterCriticalSection(&g_JobLock);
    HANDLE Job = EnsureJob();
    if (Job != nullptr)
    {
        AssignProcessToJobObject(Job, PI->hProcess); // fails harmlessly if the child is already in an incompatible job
    }
    LeaveCriticalSection(&g_JobLock);
    if (!WasSuspended)
    {
        ResumeThread(PI->hThread);
    }
    return TRUE;
}

// Ends the processes that scripts started with exec() (and what they started).
void ScriptShimTerminateProcesses()
{
    InitOnceExecuteOnce(&g_JobOnce, InitLock, nullptr, nullptr);
    EnterCriticalSection(&g_JobLock);
    if (g_Job != nullptr)
    {
        TerminateJobObject(g_Job, 1);
        CloseHandle(g_Job);
        g_Job = nullptr; // a later exec() gets a fresh job
    }
    LeaveCriticalSection(&g_JobLock);
}
