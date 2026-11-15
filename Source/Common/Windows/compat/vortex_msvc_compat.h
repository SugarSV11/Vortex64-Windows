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
using std::max;
using std::min;
#endif
