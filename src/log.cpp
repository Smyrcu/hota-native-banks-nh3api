// log.cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include "log.h"
#include "config.h"

static char g_logPath[MAX_PATH] = {0};

// Ścieżka logu = obok tej DLL.
static void ResolvePath() {
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&ResolvePath), &self);
    char dir[MAX_PATH];
    DWORD n = GetModuleFileNameA(self, dir, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) { lstrcpyA(g_logPath, "native_banks.log"); return; }
    char* slash = dir;
    for (char* p = dir; *p; ++p) if (*p == '\\' || *p == '/') slash = p + 1;
    *slash = 0;
    wsprintfA(g_logPath, "%snative_banks.log", dir);
}

void LogInit() {
    ResolvePath();
    FILE* f = fopen(g_logPath, "w");
    if (f) { fprintf(f, "[NativeBanks] log start\n"); fclose(f); }
}

static void writeLine(const char* level, const char* fmt, va_list ap) {
    FILE* f = fopen(g_logPath, "a");
    if (!f) return;
    fprintf(f, "[%s] ", level);
    vfprintf(f, fmt, ap);
    fputc('\n', f);
    fclose(f);
}

void LogInfo(const char* fmt, ...)  { va_list a; va_start(a,fmt); writeLine("INFO", fmt,a);  va_end(a); }
void LogError(const char* fmt, ...) { va_list a; va_start(a,fmt); writeLine("ERROR",fmt,a);  va_end(a); }
void LogDebug(const char* fmt, ...) {
    if (!Config::debug) return;
    va_list a; va_start(a,fmt); writeLine("DEBUG",fmt,a); va_end(a);
}
