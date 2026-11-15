// Vortex64-Windows: hooks of the script engine on Windows (the real Win32 API is used for everything else).
#include "script_win_hooks.h"

ScriptShimHooks g_ScriptShimHooks = {nullptr, nullptr};

// TODO(Windows): processes started by scripts with exec() are not tracked, so they are not ended at shutdown.
void ScriptShimTerminateProcesses()
{
}
