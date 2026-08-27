// config.cpp — INI przez GetPrivateProfile*, plik obok DLL.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "config.h"

namespace Config {
    bool enabled     = true;
    bool debug       = false;
    int  factionMode = 0;
    int  countMode   = 1;   // domyslnie: liczby po AI value
    int  factoryT7   = 2;   // domyslnie: wybor Couatl/Dreadnought (single/hotseat)
    int  startupPopup= 1;   // domyslnie: pokaz okno przy starcie
    int  language    = 0;   // domyslnie: auto (jezyk Windows)
    int  countPct[12] = {100,100,100,100,100,100,100,100,100,100,100,100};
    int  countOverride[12][7][4];
}

static const char* TOWN_KEYS[12] = {
    "Castle","Rampart","Tower","Inferno","Necropolis","Dungeon",
    "Stronghold","Fortress","Conflux","Cove","Factory","Bulwark"
};

static void IniPath(char* out, DWORD cch) {
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&IniPath), &self);
    char dir[MAX_PATH];
    DWORD n = GetModuleFileNameA(self, dir, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) { lstrcpynA(out, "native_banks.ini", cch); return; }
    char* slash = dir;
    for (char* p = dir; *p; ++p) if (*p == '\\' || *p == '/') slash = p + 1;
    *slash = 0;
    wsprintfA(out, "%snative_banks.ini", dir);
}

bool Config::Load() {
    // countOverride: globalna tablica zero-inicjalizowana, a 0 to legalna
    // wartość nadpisania — wypełnij -1 (brak) ZANIM ewentualnie wyjdziemy.
    for (int t = 0; t < 12; ++t)
        for (int tier = 0; tier < 7; ++tier)
            for (int s = 0; s < 4; ++s)
                countOverride[t][tier][s] = -1;

    char ini[MAX_PATH];
    IniPath(ini, MAX_PATH);
    if (GetFileAttributesA(ini) == INVALID_FILE_ATTRIBUTES)
        return false;   // brak pliku — domyślne
    enabled     = GetPrivateProfileIntA("NativeBanks", "Enabled",     1, ini) != 0;
    debug       = GetPrivateProfileIntA("NativeBanks", "Debug",       0, ini) != 0;
    factionMode = GetPrivateProfileIntA("NativeBanks", "FactionMode", 0, ini);
    countMode   = GetPrivateProfileIntA("NativeBanks", "CountMode",   0, ini);
    factoryT7   = GetPrivateProfileIntA("NativeBanks", "FactoryTier7", 0, ini);
    startupPopup= GetPrivateProfileIntA("NativeBanks", "StartupPopup", 1, ini);
    // Language: "auto"/"pl"/"en"/"ru" albo liczba 0-3.
    {
        char lb[16] = {0};
        GetPrivateProfileStringA("NativeBanks", "Language", "auto", lb, sizeof(lb), ini);
        char c0 = (lb[0] >= 'A' && lb[0] <= 'Z') ? lb[0] + 32 : lb[0];
        char c1 = (lb[1] >= 'A' && lb[1] <= 'Z') ? lb[1] + 32 : lb[1];
        if      (c0 == 'p' && c1 == 'l') language = 1;
        else if (c0 == 'e' && c1 == 'n') language = 2;
        else if (c0 == 'r' && c1 == 'u') language = 3;
        else if (c0 == 'u' && (c1 == 'a' || c1 == 'k')) language = 4;
        else if (c0 >= '1' && c0 <= '4' && !c1) language = c0 - '0';
        else language = 0;   // auto
    }
    for (int i = 0; i < 12; ++i) {
        int v = GetPrivateProfileIntA("CountMultiplier", TOWN_KEYS[i], 100, ini);
        if (v < 0)   v = 0;     // 0 = brak nagrody jednostkowej
        if (v > 500) v = 500;   // rozsądny sufit
        countPct[i] = v;
    }
    // [CountOverride]: "<Town>.Tier<1-7>" = c1,c2,c3,c4 (4 stany banku)
    for (int t = 0; t < 12; ++t)
        for (int tier = 0; tier < 7; ++tier) {
            for (int s = 0; s < 4; ++s) countOverride[t][tier][s] = -1;
            char key[32];
            wsprintfA(key, "%s.Tier%d", TOWN_KEYS[t], tier + 1);
            char buf[64] = {0};
            GetPrivateProfileStringA("CountOverride", key, "", buf, sizeof(buf), ini);
            if (!buf[0]) continue;
            int vals[4] = {-1,-1,-1,-1};
            int n = 0, cur = -1;
            for (char* p = buf; *p && n < 4; ++p) {
                if (*p >= '0' && *p <= '9') cur = (cur < 0 ? 0 : cur * 10) + (*p - '0');
                else if (*p == ',') { vals[n++] = cur; cur = -1; }
            }
            if (n < 4 && cur >= 0) vals[n++] = cur;
            for (int s = 0; s < 4; ++s)
                if (vals[s] >= 0 && vals[s] <= 127) countOverride[t][tier][s] = vals[s];
        }
    return true;
}
