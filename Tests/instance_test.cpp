// Single-instance handling (pjutil::RegisterRunningExe / ExistingExePid / TerminateExe) across real processes.
//   instance-test             : parent. Starts a "victim" copy of itself, checks that it is seen from outside, ends it,
//                               and checks that nothing is left over.
//   instance-test victim      : registers itself and waits to be ended.
#include <Common/Platform.h>
#include <Common/Util.h>

#include <cstdio>
#include <cstring>
#include <string>

int main(int argc, char ** argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc > 1 && strcmp(argv[1], "victim") == 0)
    {
        pjutil::RegisterRunningExe();
        for (int i = 0; i < 300; i++) // at most 30 s
        {
            Sleep(100);
        }
        return 0;
    }
    int Failures = 0;
    auto Check = [&](bool Ok, const char * What) {
        printf("%s %s\n", Ok ? "ok  " : "FAIL", What);
        Failures += Ok ? 0 : 1;
    };
    Check(pjutil::ExistingExePid() == 0, "no other copy before anything is started");

    char Self[MAX_PATH];
    GetModuleFileNameA(nullptr, Self, MAX_PATH);
    std::string Command = std::string("\"") + Self + "\" victim";
    STARTUPINFOA Si = {sizeof(Si)};
    PROCESS_INFORMATION Pi = {};
    Check(CreateProcessA(nullptr, &Command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &Si, &Pi) != 0, "victim started");
    int Seen = 0;
    for (int i = 0; i < 50 && Seen == 0; i++)
    {
        Sleep(100);
        Seen = pjutil::ExistingExePid();
    }
    Check(Seen == (int)Pi.dwProcessId, "the other copy is found with its process id");
    const bool Ended = pjutil::TerminateExe(Seen);
    Check(Ended, "the other copy was ended");
    DWORD Code = STILL_ACTIVE;
    GetExitCodeProcess(Pi.hProcess, &Code);
    Check(Code != STILL_ACTIVE, "victim process is gone");
    CloseHandle(Pi.hThread);
    CloseHandle(Pi.hProcess);
    Sleep(200);
    Check(pjutil::ExistingExePid() == 0, "nothing left over after the other copy ended (no stale registration)");
    printf("%s\n", Failures == 0 ? "PASS" : "FAILED");
    return Failures == 0 ? 0 : 1;
}
