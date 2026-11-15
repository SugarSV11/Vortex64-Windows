#include <cstring>
#include "Util.h"
#include "StdString.h"
#include "path.h"
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#ifdef _WIN32
#include "Platform.h"
void pjutil::Sleep(uint32_t timeout)
{
    ::Sleep(timeout);
}

// Vortex64-Windows single-instance handling: the running copy keeps a named file mapping (alive only as long as the
// process is) that holds its process id. No stale files: when the process dies the mapping disappears with it.
static const wchar_t * const kInstanceMapping = L"Local\\Vortex64-Windows-instance-pid";
static HANDLE g_InstanceMapping = nullptr;

static std::wstring ExePathOfProcess(HANDLE Process)
{
    wchar_t Buffer[4096];
    DWORD Size = 4096;
    return QueryFullProcessImageNameW(Process, 0, Buffer, &Size) ? std::wstring(Buffer, Size) : std::wstring();
}

int pjutil::ExistingExePid()
{
    HANDLE Map = OpenFileMappingW(FILE_MAP_READ, FALSE, kInstanceMapping);
    if (Map == nullptr)
    {
        return 0;
    }
    DWORD Pid = 0;
    if (const DWORD * View = (const DWORD *)MapViewOfFile(Map, FILE_MAP_READ, 0, 0, sizeof(DWORD)))
    {
        Pid = *View;
        UnmapViewOfFile(View);
    }
    CloseHandle(Map);
    if (Pid == 0 || Pid == GetCurrentProcessId())
    {
        return 0;
    }
    // only another copy of this executable (a recycled process id is not one)
    HANDLE Other = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Pid);
    if (Other == nullptr)
    {
        return 0;
    }
    HANDLE Self = GetCurrentProcess();
    const bool Same = _wcsicmp(ExePathOfProcess(Other).c_str(), ExePathOfProcess(Self).c_str()) == 0;
    CloseHandle(Other);
    return Same ? (int)Pid : 0;
}

struct CloseRequest
{
    DWORD Pid;
    int Posted;
};

static BOOL CALLBACK CloseWindowsOfProcess(HWND Window, LPARAM Param)
{
    CloseRequest * Request = (CloseRequest *)Param;
    DWORD WindowPid = 0;
    GetWindowThreadProcessId(Window, &WindowPid);
    if (WindowPid == Request->Pid && IsWindowVisible(Window))
    {
        PostMessageW(Window, WM_CLOSE, 0, 0);
        Request->Posted++;
    }
    return TRUE;
}

bool pjutil::TerminateExe(int Pid)
{
    if (Pid <= 0)
    {
        return false;
    }
    HANDLE Process = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, (DWORD)Pid);
    if (Process == nullptr)
    {
        return false;
    }
    // the application ends its game and saves its settings when its windows are closed: ask first, then force
    CloseRequest Request = {(DWORD)Pid, 0};
    EnumWindows(CloseWindowsOfProcess, (LPARAM)&Request);
    bool Ended = Request.Posted > 0 && WaitForSingleObject(Process, 10000) == WAIT_OBJECT_0;
    if (!Ended)
    {
        TerminateProcess(Process, 1);
        Ended = WaitForSingleObject(Process, 5000) == WAIT_OBJECT_0;
    }
    CloseHandle(Process);
    return Ended;
}

void pjutil::RegisterRunningExe()
{
    if (g_InstanceMapping != nullptr)
    {
        return;
    }
    g_InstanceMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(DWORD), kInstanceMapping);
    if (g_InstanceMapping == nullptr)
    {
        return;
    }
    if (DWORD * View = (DWORD *)MapViewOfFile(g_InstanceMapping, FILE_MAP_WRITE, 0, 0, sizeof(DWORD)))
    {
        *View = GetCurrentProcessId();
        UnmapViewOfFile(View);
    }
}
#else
void pjutil::Sleep(uint32_t timeout)
{
    int was_error;
    struct timespec elapsed, tv;
    elapsed.tv_sec = timeout / 1000;
    elapsed.tv_nsec = (timeout % 1000) * 1000000;
    do
    {
        errno = 0;
        tv.tv_sec = elapsed.tv_sec;
        tv.tv_nsec = elapsed.tv_nsec;
        was_error = nanosleep(&tv, &elapsed);
    } while (was_error && (errno == EINTR));
}


static std::string RunningExeFile()
{
    const char * Dir = getenv("XDG_RUNTIME_DIR");
    return std::string(Dir != nullptr && *Dir != 0 ? Dir : "/tmp") + "/vortex64-" + std::to_string((long)getuid()) + ".pid";
}

static std::string ExePathOf(long pid)
{
    char Buffer[4096];
    ssize_t Len = readlink(("/proc/" + std::to_string(pid) + "/exe").c_str(), Buffer, sizeof(Buffer) - 1);
    return Len > 0 ? std::string(Buffer, (size_t)Len) : std::string();
}

// The pid file is locked (flock) by the running copy for as long as it runs: two copies started at once cannot both
// believe that they are the only one, and a file that a copy left behind is not a running copy.
static int g_PidFileFd = -1;

static bool LockPidFile(bool Wait)
{
    if (g_PidFileFd >= 0)
    {
        return true;
    }
    const int Fd = open(RunningExeFile().c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (Fd < 0)
    {
        return false;
    }
    for (int i = 0; i < (Wait ? 50 : 1); i++)
    {
        if (flock(Fd, LOCK_EX | LOCK_NB) == 0)
        {
            g_PidFileFd = Fd;
            return true;
        }
        if (Wait)
        {
            pjutil::Sleep(100);
        }
    }
    close(Fd);
    return false;
}

int pjutil::ExistingExePid()
{
    if (LockPidFile(false))
    {
        return 0; // nobody else holds it: this copy is the only one (and keeps the lock)
    }
    // another copy has the lock; it writes its process id just after it has it
    long pid = 0;
    for (int i = 0; i < 10 && pid <= 0; i++)
    {
        FILE * File = fopen(RunningExeFile().c_str(), "r");
        if (File != nullptr)
        {
            if (fscanf(File, "%ld", &pid) != 1)
            {
                pid = 0;
            }
            fclose(File);
        }
        if (pid <= 0)
        {
            Sleep(100);
        }
    }
    // only another copy of this executable (a recycled process id is not one)
    if (pid <= 0 || pid == (long)getpid() || kill((pid_t)pid, 0) != 0 || ExePathOf(pid).empty() || ExePathOf(pid) != ExePathOf((long)getpid()))
    {
        return 0;
    }
    return (int)pid;
}

bool pjutil::TerminateExe(int pid)
{
    if (pid <= 0 || kill((pid_t)pid, SIGTERM) != 0)
    {
        return false;
    }
    // the application ends its game and saves its settings on SIGTERM
    for (int i = 0; i < 100 && kill((pid_t)pid, 0) == 0; i++)
    {
        Sleep(100);
    }
    if (kill((pid_t)pid, 0) == 0)
    {
        kill((pid_t)pid, SIGKILL);
        for (int i = 0; i < 20 && kill((pid_t)pid, 0) == 0; i++)
        {
            Sleep(100);
        }
    }
    return kill((pid_t)pid, 0) != 0;
}

void pjutil::RegisterRunningExe()
{
    // the lock is taken (waiting for the copy that was ended to let go of it) and the process id written
    if (!LockPidFile(true))
    {
        return;
    }
    if (ftruncate(g_PidFileFd, 0) == 0)
    {
        const std::string Text = std::to_string((long)getpid()) + "\n";
        if (pwrite(g_PidFileFd, Text.c_str(), Text.size(), 0) < 0)
        {
            // (the file is only informational)
        }
    }
}

#endif
