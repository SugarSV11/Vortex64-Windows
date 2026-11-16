#pragma once
// Vortex64-Windows compat: minimal dlfcn over LoadLibrary (UTF-8 paths)
#include <windows.h>
#include <string>

#define RTLD_NOW 0
#define RTLD_LAZY 0
#define RTLD_LOCAL 0
#define RTLD_GLOBAL 0
#define RTLD_NOLOAD 0

struct Dl_info
{
    const char * dli_fname;
    void * dli_fbase;
    const char * dli_sname;
    void * dli_saddr;
};

static inline void * dlopen(const char * path, int)
{
    if (path == nullptr) return (void *)GetModuleHandleW(nullptr);
    int n = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    if (n <= 0) return nullptr;
    std::wstring w(n - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path, -1, &w[0], n);
    return (void *)LoadLibraryW(w.c_str());
}
static inline void * dlsym(void * h, const char * name) { return (void *)GetProcAddress((HMODULE)h, name); }
static inline int dlclose(void * h) { return FreeLibrary((HMODULE)h) ? 0 : 1; }
static inline const char * dlerror() { return "Windows LoadLibrary/GetProcAddress error"; }
static inline int dladdr(const void * addr, Dl_info * info)
{
    static thread_local char fname[MAX_PATH * 4];
    HMODULE mod = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)addr, &mod))
        return 0;
    wchar_t w[MAX_PATH];
    DWORD len = GetModuleFileNameW(mod, w, MAX_PATH);
    if (len == 0) return 0;
    int n = WideCharToMultiByte(CP_UTF8, 0, w, (int)len, fname, (int)sizeof(fname) - 1, nullptr, nullptr);
    fname[n > 0 ? n : 0] = '\0';
    info->dli_fname = fname;
    info->dli_fbase = (void *)mod;
    info->dli_sname = nullptr;
    info->dli_saddr = nullptr;
    return 1;
}
