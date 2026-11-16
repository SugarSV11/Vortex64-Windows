#pragma once
// Vortex64-Windows: GCC/POSIX names used by shared code, mapped to MSVC equivalents.
// Force-included into every C++ translation unit when building with MSVC (see root CMakeLists.txt).
#ifdef _MSC_VER
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#define __builtin_bswap16(x) _byteswap_ushort(x)
#define __builtin_bswap32(x) _byteswap_ulong(x)
#define __builtin_bswap64(x) _byteswap_uint64(x)
#define strtok_r(s, d, p) strtok_s((s), (d), (p))
// POSIX setenv (overwrite=0 keeps an existing value)
static inline int setenv(const char * Name, const char * Value, int Overwrite)
{
    if (!Overwrite && getenv(Name) != nullptr)
    {
        return 0;
    }
    return _putenv_s(Name, Value);
}
using std::max;
using std::min;
#endif
