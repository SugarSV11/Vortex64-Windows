#pragma once
// Vortex64-Windows: header-only crash reporter. Prints a symbolized stack (needs the .pdb next to the executable) for
//   - unhandled SEH exceptions (walked from the faulting context, also for calls through null pointers),
//   - std::terminate, abort() (SIGABRT), invalid CRT parameters and pure virtual calls.
// It also turns off the Windows error dialogs for the process. Used by the tests so that a failure that would otherwise end
// the process silently (e.g. 0xC0000409) or open a dialog on the desktop is printed instead.
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdlib.h>
#include <crtdbg.h>
#pragma comment(lib, "dbghelp.lib")

namespace CrashReport
{
inline void PrintFrame(HANDLE Process, int Index, DWORD64 Address)
{
    char Buf[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO * S = (SYMBOL_INFO *)Buf;
    S->SizeOfStruct = sizeof(SYMBOL_INFO);
    S->MaxNameLen = 255;
    DWORD64 Disp = 0;
    IMAGEHLP_LINE64 Line = {sizeof(Line)};
    DWORD LineDisp = 0;
    if (SymFromAddr(Process, Address, &Disp, S))
    {
        if (SymGetLineFromAddr64(Process, Address, &LineDisp, &Line))
            fprintf(stderr, "  #%d %s (%s:%lu)\n", Index, S->Name, Line.FileName, Line.LineNumber);
        else
            fprintf(stderr, "  #%d %s +0x%llx\n", Index, S->Name, (unsigned long long)Disp);
    }
    else
        fprintf(stderr, "  #%d %p\n", Index, (void *)Address);
}

inline void PrintStack(const char * Why)
{
    fprintf(stderr, "\n[CRASH] %s\n", Why);
    HANDLE Process = GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    SymInitialize(Process, nullptr, TRUE);
    void * Frames[64];
    const USHORT N = CaptureStackBackTrace(1, 64, Frames, nullptr);
    for (USHORT i = 0; i < N; i++)
    {
        PrintFrame(Process, i, (DWORD64)Frames[i]);
    }
    fflush(stderr);
}

// the stack of the faulting thread, walked from the context of the exception
inline void PrintStackFromContext(CONTEXT Ctx)
{
    HANDLE Process = GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    SymInitialize(Process, nullptr, TRUE);
    if (Ctx.Rip == 0)
    {
        // a call through a null pointer: the return address is at the top of the stack
        Ctx.Rip = *(DWORD64 *)Ctx.Rsp;
        Ctx.Rsp += 8;
        fprintf(stderr, "  (call through a null pointer; its caller follows)\n");
    }
    STACKFRAME64 Sf = {};
    Sf.AddrPC.Offset = Ctx.Rip;
    Sf.AddrFrame.Offset = Ctx.Rsp;
    Sf.AddrStack.Offset = Ctx.Rsp;
    Sf.AddrPC.Mode = Sf.AddrFrame.Mode = Sf.AddrStack.Mode = AddrModeFlat;
    for (int i = 0; i < 48; i++)
    {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, Process, GetCurrentThread(), &Sf, &Ctx, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr) || Sf.AddrPC.Offset == 0)
        {
            break;
        }
        PrintFrame(Process, i, Sf.AddrPC.Offset);
    }
    fflush(stderr);
}

inline LONG WINAPI OnException(EXCEPTION_POINTERS * Ep)
{
    fprintf(stderr, "\n[CRASH] unhandled exception 0x%08lX at %p\n", Ep->ExceptionRecord->ExceptionCode, Ep->ExceptionRecord->ExceptionAddress);
    PrintStackFromContext(*Ep->ContextRecord);
    return EXCEPTION_EXECUTE_HANDLER;
}
inline void OnTerminate()
{
    PrintStack("std::terminate (uncaught C++ exception or noexcept violation)");
    _exit(3);
}
inline void OnAbort(int)
{
    PrintStack("abort()");
    _exit(3);
}
inline void OnInvalidParameter(const wchar_t * Expr, const wchar_t * Func, const wchar_t * File, unsigned Line, uintptr_t)
{
    fprintf(stderr, "\n[CRASH] invalid CRT parameter: %ls in %ls (%ls:%u)\n", Expr ? Expr : L"?", Func ? Func : L"?", File ? File : L"?", Line);
    PrintStack("invalid parameter");
    _exit(3);
}
inline void OnPureCall()
{
    PrintStack("pure virtual function call");
    _exit(3);
}

inline void Install()
{
    // no Windows error dialogs (crash, missing file, critical errors): the handlers below print a stack instead
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    SetUnhandledExceptionFilter(OnException);
    std::set_terminate(OnTerminate);
    signal(SIGABRT, OnAbort);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _set_invalid_parameter_handler(OnInvalidParameter);
    _set_purecall_handler(OnPureCall);
}
} // namespace CrashReport
#endif
