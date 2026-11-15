#pragma once
// Vortex64-Windows compat: minimal dirent over FindFirstFileW (UTF-8 names)
#include <windows.h>
#include <string>
#include <cstring>

#define DT_DIR 4
#define DT_REG 8

struct dirent
{
    unsigned char d_type;
    char d_name[1024];
};

struct DIR
{
    HANDLE h;
    WIN32_FIND_DATAW data;
    bool first;
    dirent ent;
};

static inline DIR * opendir(const char * path)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    if (n <= 0) return nullptr;
    std::wstring w(n - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path, -1, &w[0], n);
    if (!w.empty() && (w.back() == L'/' || w.back() == L'\\')) w.pop_back();
    w += L"/*";
    DIR * d = new DIR();
    d->h = FindFirstFileW(w.c_str(), &d->data);
    if (d->h == INVALID_HANDLE_VALUE) { delete d; return nullptr; }
    d->first = true;
    return d;
}

static inline dirent * readdir(DIR * d)
{
    if (d->first) d->first = false;
    else if (!FindNextFileW(d->h, &d->data)) return nullptr;
    d->ent.d_type = (d->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;
    WideCharToMultiByte(CP_UTF8, 0, d->data.cFileName, -1, d->ent.d_name, sizeof(d->ent.d_name), nullptr, nullptr);
    return &d->ent;
}

static inline int closedir(DIR * d)
{
    FindClose(d->h);
    delete d;
    return 0;
}
