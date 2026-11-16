#include "stdafx.h"

#include "ScriptAPI.h"
#include <windows.h>

void ScriptAPI::Define_exec(duk_context * ctx)
{
    DefineGlobalFunction(ctx, "exec", js_exec);
}

duk_ret_t ScriptAPI::js_exec(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_String, Arg_OptObject});

    CScriptInstance * inst = GetInstance(ctx);
    CScriptSystem * sys = inst->System();

    const char * command = duk_get_string(ctx, 0);

    struct
    {
        bool bShowWindow = false;
        bool bVerbose = false;
        stdstr cwd;
    } options;

    options.cwd = sys->InstallDirPath();

    if (duk_is_object(ctx, 1))
    {
        duk_get_prop_string(ctx, 1, "hidden");
        options.bShowWindow = duk_get_boolean_default(ctx, -1, false) != 0;
        duk_pop(ctx);

        duk_get_prop_string(ctx, 1, "verbose");
        options.bVerbose = duk_get_boolean_default(ctx, -1, false) != 0;
        duk_pop(ctx);

        if (duk_has_prop_string(ctx, 1, "cwd"))
        {
            duk_get_prop_string(ctx, 1, "cwd");
            options.cwd = duk_get_string(ctx, -1);
            duk_pop(ctx);
        }
    }

    stdstr resultStdOut;
    stdstr resultStdErr;
    DWORD resultExitCode = EXIT_FAILURE;

    SECURITY_ATTRIBUTES secAttr;
    secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    secAttr.bInheritHandle = TRUE;
    secAttr.lpSecurityDescriptor = nullptr;

    HANDLE hStdIn_r, hStdIn_w;
    HANDLE hStdOut_r, hStdOut_w;
    HANDLE hStdErr_r, hStdErr_w;

    CreatePipe(&hStdIn_r, &hStdIn_w, &secAttr, 0);
    CreatePipe(&hStdOut_r, &hStdOut_w, &secAttr, 0);
    CreatePipe(&hStdErr_r, &hStdErr_w, &secAttr, 0);

    STARTUPINFOA startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdInput = hStdIn_r;
    startupInfo.hStdOutput = hStdOut_w;
    startupInfo.hStdError = hStdErr_w;

    if (!options.bShowWindow)
    {
        startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
        startupInfo.wShowWindow = SW_HIDE;
    }

    PROCESS_INFORMATION processInfo;

    BOOL bSuccess = CreateProcessA(nullptr, (LPSTR)stdstr_f("cmd /c %s", command).c_str(), nullptr, nullptr, TRUE,
                                   0, nullptr, options.cwd.c_str(), &startupInfo, &processInfo);

    if (bSuccess)
    {
        CloseHandle(hStdIn_r);
        CloseHandle(hStdIn_w);
        CloseHandle(hStdOut_w);
        CloseHandle(hStdErr_w);

        // Both pipes are read while the shell runs, and the reading ends when the shell has ended and the pipes are
        // empty. (Waiting for the end of the pipes would also wait for what the shell started in the background,
        // e.g. "start /b prog", because those processes inherit the pipe handles; on Linux they are not waited for.)
        char buffer[1024];
        auto Drain = [&](HANDLE Pipe, stdstr & Result, bool Echo) {
            bool GotData = false;
            DWORD nBytesAvail = 0, nBytesRead = 0;
            while (PeekNamedPipe(Pipe, nullptr, 0, nullptr, &nBytesAvail, nullptr) && nBytesAvail != 0)
            {
                if (!ReadFile(Pipe, buffer, sizeof(buffer), &nBytesRead, nullptr) || nBytesRead == 0)
                {
                    break;
                }
                GotData = true;
                Result.append(buffer, nBytesRead);
                if (Echo && options.bVerbose)
                {
                    sys->ConsolePrint("%.*s", nBytesRead, buffer);
                }
            }
            return GotData;
        };
        for (;;)
        {
            const bool Exited = WaitForSingleObject(processInfo.hProcess, 0) == WAIT_OBJECT_0;
            const bool GotOut = Drain(hStdOut_r, resultStdOut, true);
            const bool GotErr = Drain(hStdErr_r, resultStdErr, false);
            if (Exited && !GotOut && !GotErr)
            {
                break; // (data written before the shell ended was read by the Drain above this check's wait)
            }
            if (!GotOut && !GotErr)
            {
                WaitForSingleObject(processInfo.hProcess, 5);
            }
        }

        CloseHandle(hStdErr_r);
        CloseHandle(hStdOut_r);
        CloseHandle(processInfo.hThread);

        WaitForSingleObject(processInfo.hProcess, INFINITE);
        GetExitCodeProcess(processInfo.hProcess, &resultExitCode);

        CloseHandle(processInfo.hProcess);
    }
    else
    {
        CloseHandle(hStdIn_r);
        CloseHandle(hStdIn_w);
        CloseHandle(hStdOut_r);
        CloseHandle(hStdOut_w);
        CloseHandle(hStdErr_r);
        CloseHandle(hStdErr_w);
    }

    if (!bSuccess || resultExitCode != 0)
    {
        const DukPropListEntry props[] = {
            {"status", DukInt(resultExitCode)},
            {"stdout", DukString(resultStdOut.c_str())},
            {"stderr", DukString(resultStdErr.c_str())},
            {"pid", DukUInt(processInfo.dwProcessId)},
            {nullptr},
        };

        duk_push_error_object(ctx, resultExitCode, "exec(): process returned %d", resultExitCode);
        DukPutPropList(ctx, -1, props);
        return duk_throw(ctx);
    }
    else
    {
        duk_push_string(ctx, resultStdOut.c_str());
    }

    return 1;
}