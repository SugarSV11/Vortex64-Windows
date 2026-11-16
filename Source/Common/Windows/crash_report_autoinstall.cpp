// Vortex64-Windows: linked into the test executables. Installs the crash reporter before main(), so a crashing test prints a
// symbolized stack instead of ending silently or opening a Windows error dialog on the desktop.
#include "compat/crash_report.h"

namespace
{
const int g_Installed = (CrashReport::Install(), 0);
}
