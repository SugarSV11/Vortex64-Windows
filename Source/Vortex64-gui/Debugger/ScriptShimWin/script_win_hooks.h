#pragma once
// Vortex64-Windows: force-included into the script-engine sources. The script API posts window messages to the
// main window and shows message boxes; the Qt frontend handles both through hooks (see ScriptGlue.cpp).
#include <windows.h>
#include <shlwapi.h> // PathFileExistsA
#include <stdint.h>
#undef PostMessage
#undef MessageBox

struct ScriptShimHooks
{
    void (*PostMessage)(unsigned Message, uintptr_t WParam, intptr_t LParam);
    void (*MessageBox)(const char * Text, const char * Caption);
};
extern ScriptShimHooks g_ScriptShimHooks;
void ScriptShimTerminateProcesses();

inline BOOL ScriptShimPostMessageImpl(UINT Message, WPARAM WParam, LPARAM LParam)
{
    if (g_ScriptShimHooks.PostMessage != nullptr)
    {
        g_ScriptShimHooks.PostMessage(Message, (uintptr_t)WParam, (intptr_t)LParam);
    }
    return TRUE;
}
inline int ScriptShimMessageBoxImpl(const char * Text, const char * Caption)
{
    if (g_ScriptShimHooks.MessageBox != nullptr)
    {
        g_ScriptShimHooks.MessageBox(Text, Caption);
    }
    return 1;
}
// Processes started by exec() join a Job Object (KILL_ON_JOB_CLOSE), so ScriptShimTerminateProcesses (and the end
// of the GUI) ends them together with what they started.
BOOL ScriptShimCreateProcessImpl(LPCSTR App, LPSTR Cmd, LPSECURITY_ATTRIBUTES PA, LPSECURITY_ATTRIBUTES TA, BOOL Inherit,
                                 DWORD Flags, LPVOID Env, LPCSTR Dir, LPSTARTUPINFOA SI, LPPROCESS_INFORMATION PI);
#define CreateProcessA(a, c, pa, ta, i, f, e, d, s, p) ScriptShimCreateProcessImpl((a), (c), (pa), (ta), (i), (f), (e), (d), (s), (p))

#define PostMessage(h, m, w, l) ScriptShimPostMessageImpl((m), (w), (l))
#define PostMessageA(h, m, w, l) ScriptShimPostMessageImpl((m), (w), (l))
#define MessageBoxA(h, t, c, f) ScriptShimMessageBoxImpl((t), (c))
