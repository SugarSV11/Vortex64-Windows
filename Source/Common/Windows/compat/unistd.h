#pragma once
// Vortex64-Windows compat: minimal subset of <unistd.h> used by Common
#include <direct.h>
#include <io.h>
#include <process.h>
#define getcwd _getcwd
#define chdir _chdir
#define getpid _getpid
#define close _close
#define W_OK 2
#define R_OK 4
#define access _access
#ifndef S_ISDIR
#include <sys/stat.h>
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif
#include <windows.h>
static inline int usleep(unsigned long us) { ::Sleep((DWORD)((us + 999) / 1000)); return 0; }
