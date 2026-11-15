#pragma once
// Vortex64-Windows compat: MSVC has no strings.h
#include <string.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
