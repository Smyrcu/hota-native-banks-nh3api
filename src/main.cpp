// main.cpp — HotA "Native Bank Rewards" plugin (wariant na NH3API).
// LoHook na FUN_004abab0 (handler nagrody banku): podmienia creatureRewardType
// na jednostkę native frakcji bohatera odbierającego nagrodę, tier→tier.
// Offsety HotA 1.8.0 wyRE-owane ręcznie (../findings.md) — NH3API dostarcza tu
// tylko patcher (GetPatcher/HookContext/LoHook); struktury HotA są adresowane
// surowo przez g_moduleBase + rva::*. Tabele: mapping.cpp / ../mapping.md.
#define WIN32_LEAN_AND_MEAN
#include "version.h"
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "nh3api/core/nh3api_std/patcher_x86.hpp"  // NH3API patcher (GetPatcher/HookContext/LoHook)
#include "mapping.h"
#include "config.h"
#include "log.h"

using namespace nb;

// --- RVA (dla HotA 1.8.0, hash w findings.md) ---
namespace rva {
    constexpr uintptr_t SIG_ENTRY     = 0xabab0;  // FUN_004abab0 (entry, sygnatura)
    constexpr uintptr_t HOOK_AT       = 0xabbfa;  // punkt wpięcia: ścieżka ZWYCIĘSTWA, po walce
                                                  // (JZ 0x4abbfa po teście wyniku FUN_004ad160;
                                                  // EBX=bank i EBP ramka nadal ważne)
    constexpr uintptr_t MAIN_PTR      = 0x299538; // H3Main* global (VA 0x699538)
    constexpr uintptr_t PLAYERS_INFO  = 0x1F6A0;  // H3Main + = H3PlayersInfo
    constexpr uintptr_t TOWN_TYPE     = 0x10;     // H3PlayersInfo + = INT32 townType[8]
    constexpr uintptr_t BANK_SETUPS   = 0x295088; // tabela 11×H3CreatureBankSetup (VA 0x695088; w HotA wyzerowana)
    constexpr uintptr_t CREATURE_TBL  = 0x2747b0; // H3CreatureInformation* global (VA 0x6747B0)
    constexpr uintptr_t DLG_NORMAL    = 0xf6c00;  // NormalDialog (VA 0x4F6C00, __fastcall, 12 arg)
    constexpr uintptr_t WNDMGR_PTR    = 0x2992d0; // H3WindowManager* global (VA 0x6992D0)
    constexpr uintptr_t PLAYERS       = 0x20AD0;  // H3Main + = H3Player[8] (stride 0x168)
    constexpr uintptr_t CUR_PLAYER    = 0x29CCF4; // ID aktywnego gracza na TYM kliencie (VA 0x69CCF4)
    constexpr uintptr_t GENRL_TXT     = 0x2A5DC4; // H3TextFile* genrltxt.txt (VA 0x6A5DC4)
}

static uintptr_t g_moduleBase = 0;

// --- Lokalizacja komunikatów (PL/EN/RU) ---
// Język bierzemy z GRY, nie z Windows: skan tabeli genrltxt.txt w pamięci
// (H3TextFile* @ VA 0x6A5DC4) za słowem "Cancel"/"Anuluj"/"Отмен…".
// Nierozpoznany język paczki (FR/ES/…) albo brak tabeli -> EN.
// Popup startowy: MessageBoxW (Unicode — cyrylica niezależna od codepage
// systemu). Dialog w grze: silnik H3 renderuje ANSI własnym fontem, więc RU
// podajemy w CP1251 (font wersji RU), a PL bez ogonków (font EN ich nie ma).
namespace lang { enum { PL = 1, EN = 2, RU = 3, UA = 4 }; }
static int g_lang = 0;   // 0 = jeszcze nie wykryto

static int DetectGameLang() {
    if (!g_moduleBase) return 0;
    uintptr_t tf = *reinterpret_cast<uintptr_t*>(g_moduleBase + rva::GENRL_TXT);
    if (!tf) return 0;
    // H3TextFile: H3Vector<LPCSTR> text @ +0x1C (m_first @ +0x20, m_end @ +0x24)
    const char** first = *reinterpret_cast<const char***>(tf + 0x20);
    const char** end   = *reinterpret_cast<const char***>(tf + 0x24);
    if (!first || !end || end <= first || end - first > 10000) return 0;
    bool en = false;
    for (const char** p = first; p < end; ++p) {
        const char* s = *p;
        if (!s) continue;
        if (!strcmp(s, "Anuluj")) return lang::PL;
        if (!strncmp(s, "\xCE\xF2\xEC\xE5\xED", 5)) return lang::RU;      // "Отмен" CP1251
        if (!strncmp(s, "\xD1\xEA\xE0\xF1\xF3\xE2", 6)) return lang::UA;  // "Скасув" CP1251
        if (!strcmp(s, "Cancel")) en = true;
    }
    return en ? lang::EN : 0;
}

// 0 = wymuszenie z INI nieaktywne i genrltxt jeszcze nie wczytany.
static int TryLang() {
    if (Config::language >= lang::PL && Config::language <= lang::UA)
        return Config::language;
    if (!g_lang) g_lang = DetectGameLang();
    return g_lang;
}
static int CurLang() { int l = TryLang(); return l ? l : lang::EN; }

static const wchar_t* PopupText() {
    switch (CurLang()) {
    case lang::PL: return
        L"Native Bank Rewards v" NB_VERSION_WSTR L" — plugin aktywny.\n\n"
        L"Nagrody jednostkowe z creature banków są podmieniane na\n"
        L"jednostki native Twojej frakcji.\n\n"
        L"Gra ZMODOWANA. Online: wszyscy gracze muszą mieć ten plugin.";
    case lang::RU: return
        L"Native Bank Rewards v" NB_VERSION_WSTR L" — плагин активен.\n\n"
        L"Существа-награды из банков существ заменяются на\n"
        L"существ вашей родной фракции.\n\n"
        L"Игра МОДИФИЦИРОВАНА. Онлайн: у всех игроков должен быть этот плагин.";
    case lang::UA: return
        L"Native Bank Rewards v" NB_VERSION_WSTR L" — плагін активний.\n\n"
        L"Юніти-нагороди з creature banks замінюються на\n"
        L"рідних юнітів вашої фракції.\n\n"
        L"Гру МОДИФІКОВАНО. Онлайн: плагін потрібен усім гравцям.";
    default: return
        L"Native Bank Rewards v" NB_VERSION_WSTR L" — plugin active.\n\n"
        L"Creature rewards from creature banks are replaced with\n"
        L"units native to your faction.\n\n"
        L"Game is MODDED. Online: all players must have this plugin.";
    }
}

static const char* ChooseRewardText() {
    switch (CurLang()) {
    case lang::PL: return "Wybierz nagrode:";
    case lang::RU: return "\xC2\xFB\xE1\xE5\xF0\xE8\xF2\xE5 \xED\xE0\xE3\xF0\xE0\xE4\xF3:";  // CP1251
    case lang::UA: return "\xC2\xE8\xE1\xE5\xF0\xB3\xF2\xFC \xED\xE0\xE3\xEE\xF0\xEE\xE4\xF3:";  // "Виберіть нагороду:" CP1251
    default:       return "Choose your reward:";
    }
}

// Rozpoznanie stanu banku (0..3) po (oryginalny typ nagrody, liczba).
// Tabela z opisów banków w HotA.dat (HotA 1.8.0) — exe-owa tabela SoD jest
// w HotA wyzerowana (HotA trzyma definicje we własnych strukturach).
// Wszystkie banki HotA 1.8 z nagrodą jednostkową:
struct RewardStates { int16_t type; int8_t counts[4]; };
static const RewardStates REWARD_STATES[] = {
    { 12,  {1, 2, 3, 4} },   // Griffin Conservatory -> Angel        (t7)
    { 108, {4, 6, 8, 12} },  // Dragon Fly Hive      -> Wyvern       (t6)
    { 94,  {4, 6, 8, 12} },  // Wolf Raider Picket   -> Cyclops      (t6)
    { 40,  {1, 2, 3, 4} },   // Experimental Shop    -> Giant        (t7)
    { 130, {1, 2, 3, 4} },   // Red Tower            -> Firebird     (t7)
    { 165, {1, 2, 3, 4} },   // Pirate Cavern        -> Sea Serpent  (t7)
    { 136, {3, 6, 9, 12} },  // Ivory Tower          -> Enchanter    (neutral)
};
static const int8_t* FindRewardCounts(int rewardType) {
    for (const auto& r : REWARD_STATES)
        if (r.type == rewardType) return r.counts;
    return nullptr;
}
static int FindBankStateIdx(int rewardType, int rewardCount) {
    const int8_t* cs = FindRewardCounts(rewardType);
    if (cs)
        for (int s = 0; s < 4; ++s)
            if (cs[s] == rewardCount) return s;
    return -1;
}

// AI value jednostki z żywej tabeli kreatur gry (stride 0x74, aiValue @ +0x40).
// Działa też dla jednostek HotA (>150) — tabela jest rozszerzona przez HotA.dll.
static int AiValue(int creatureId) {
    if (creatureId < 0) return -1;
    uintptr_t base = *reinterpret_cast<uintptr_t*>(g_moduleBase + rva::CREATURE_TBL);
    if (!base) return -1;
    return *reinterpret_cast<int32_t*>(base + 0x74u * creatureId + 0x40);
}

// Sygnatura bajtowa entry FUN_004abab0 (?? = zamaskowane operandy abs/rel).
static const int SIG_LEN = 40;
static const uint8_t SIG[SIG_LEN] = {
    0x55,0x8B,0xEC,0x6A,0xFF,0x68,0,0,0,0, 0x64,0xA1,0x00,0x00,0x00,0x00,
    0x50,0x64,0x89,0x25,0x00,0x00,0x00,0x00, 0x83,0xEC,0x68,0x53,0x56,0x57,
    0x8B,0xF9,0x8B,0x4D,0x0C,0xE8,0,0,0,0
};
static const bool SIG_MASK[SIG_LEN] = {
    1,1,1,1,1,1,0,0,0,0, 1,1,1,1,1,1,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,
    1,1,1,1,1,1,0,0,0,0
};


// Odczyt frakcji bohatera: hero_class @ +0x30, town = class/2.
static int HeroTown(void* hero) {
    if (!hero) return TOWN_INVALID;
    int heroClass = *reinterpret_cast<int*>(reinterpret_cast<char*>(hero) + 0x30);
    if (heroClass < 0) return TOWN_INVALID;
    int town = heroClass / 2;
    if (town < 0 || town >= TOWN_COUNT) return TOWN_INVALID; // HotA class niepewny -> skip
    return town;
}

// --- Hook ---
static int __stdcall OnBankReward(LoHook* /*h*/, HookContext* c) {
    // Wpięcie @0xabbfa (po zwycięskiej walce): EBX = instancja banku, ramka gry w ebp:
    // [ebp+4] = ret addr wołającego, [ebp+8] = param_1 = hero.
    uintptr_t ebp = static_cast<uintptr_t>(c->ebp);
    uintptr_t esp = static_cast<uintptr_t>(c->esp);
    // sanity ramki (ebp stabilny przez całą funkcję; esp zależy od miejsca wpięcia)
    if (ebp <= esp || ebp - esp > 0x200) {
        LogDebug("HOOK: nieoczekiwana ramka (ebp-esp=0x%x) — pomijam", (unsigned)(ebp - esp));
        return EXEC_DEFAULT;
    }
    uintptr_t caller = *reinterpret_cast<uintptr_t*>(ebp + 4);
    uint8_t*  hero   = *reinterpret_cast<uint8_t**>(ebp + 8);
    uint8_t*  bank   = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(c->ebx));

    if (!bank || !hero) return EXEC_DEFAULT;
    int  origType = *reinterpret_cast<int32_t*>(bank + 0x54);
    int  count    = *reinterpret_cast<int8_t*>(bank + 0x58);
    int  heroCls  = *reinterpret_cast<int32_t*>(hero + 0x30);
    int  owner    = *reinterpret_cast<int8_t*>(hero + 0x22);

    // Frakcja GRACZA (startowe miasto): H3Main->playersInfo.townType[owner].
    int playerTown = TOWN_INVALID;
    if (owner >= 0 && owner < 8) {
        uintptr_t mainPtr = *reinterpret_cast<uintptr_t*>(g_moduleBase + rva::MAIN_PTR);
        if (mainPtr)
            playerTown = *reinterpret_cast<int32_t*>(
                mainPtr + rva::PLAYERS_INFO + rva::TOWN_TYPE + 4u * owner);
    }
    int heroTown = (heroCls >= 0) ? heroCls / 2 : TOWN_INVALID;

    LogDebug("HOOK: caller=%x hero=%x cls=%d owner=%d playerTown=%d bank=%x type=%d count=%d",
             (unsigned)caller,(unsigned)(uintptr_t)hero,heroCls,owner,playerTown,
             (unsigned)(uintptr_t)bank,origType,count);

    // Bez gate'a na callera: hook siedzi WEWNĄTRZ funkcji nagrody, po tym jak
    // sama wyliczyła EBX = instancja banku — każde wywołanie to realny grant.
    // (HotA.dll przekierowuje ścieżkę wywołania, więc caller bywa w HotA.dll.)
    (void)caller;
    if (count <= 0) return EXEC_DEFAULT;

    // FactionMode 0 = frakcja gracza (startowe miasto; fallback: klasa bohatera),
    //             1 = frakcja klasy bohatera odbierającego nagrodę.
    int town;
    if (Config::factionMode == 1)
        town = heroTown;
    else
        town = (playerTown >= 0 && playerTown < TOWN_COUNT) ? playerTown : heroTown;
    if (town < 0 || town >= TOWN_COUNT) {
        LogDebug("remap: town poza zakresem (cls=%d playerTown=%d) — pomijam", heroCls, playerTown);
        return EXEC_DEFAULT;
    }
    int native = RemapReward(origType, town);
    if (native >= 0) {
        LogDebug("remap: town=%d type %d -> %d (count=%d)", town, origType, native, count);
        *reinterpret_cast<int32_t*>(bank + 0x54) = native;   // dialog + grant użyją nowego
    } else {
        LogDebug("remap: brak mapowania (type=%d town=%d) — typ zostaje", origType, town);
    }

    // FactoryTier7=2: wybór gracza (dialog TAKE, dwie ikonki kreatur).
    // Tylko dla gracza-człowieka; AI dostaje Couatla (default z mapping).
    if (native >= 0 && town == TOWN_FACTORY && Config::factoryT7 == 2) {
        int st2, tr2, up2;
        if (ReverseLookup(origType, st2, tr2, up2) && tr2 == 6) {
            // Dialog TYLKO na kliencie gracza zdobywającego bank:
            //  - owner == aktywny gracz na tym kliencie (CurrentPlayerID)
            //  - gracz jest człowiekiem (AI dostaje default bez dialogu)
            // To gwarantuje: brak dialogu u przeciwnika (jego CurrentPlayerID !=
            // owner) => nie zdradza ruchu i nie hanguje/desyncuje jego klienta.
            int curPlayer = *reinterpret_cast<int32_t*>(g_moduleBase + rva::CUR_PLAYER);
            bool human = false;
            uintptr_t mainPtr = *reinterpret_cast<uintptr_t*>(g_moduleBase + rva::MAIN_PTR);
            if (mainPtr && owner >= 0 && owner < 8)
                human = *reinterpret_cast<int8_t*>(
                    mainPtr + rva::PLAYERS + 0x168u * owner + 0xE1) != 0;
            if (human && owner == curPlayer) {
                int left  = up2 ? 183 : 182;   // Couatl / Crimson Couatl
                int right = up2 ? 185 : 184;   // Dreadnought / Juggernaut
                typedef void (__fastcall* DlgFn)(const char*, int, int, int,
                    int, int, int, int, int, int, int, int);
                DlgFn dlg = reinterpret_cast<DlgFn>(g_moduleBase + rva::DLG_NORMAL);
                dlg(ChooseRewardText(), 7 /*TAKE*/, -1, -1,
                    21 /*CREATURE*/, left, 21, right, -1, 0, -1, 0);
                uintptr_t wm = *reinterpret_cast<uintptr_t*>(g_moduleBase + rva::WNDMGR_PTR);
                int clicked = wm ? *reinterpret_cast<int32_t*>(wm + 0x38) : 0;
                // UWAGA: w HotA/HD id są odwrócone względem nazw H3API:
                // 30730 = kliknięty PRAWY obrazek, 30729 = LEWY (zweryfikowane w grze).
                native = (clicked == 30730) ? right : left;
                *reinterpret_cast<int32_t*>(bank + 0x54) = native;
                LogDebug("factoryT7 dialog: clicked=%d -> native=%d", clicked, native);
            }
        }
    }

    // Korekta liczby jednostek — tylko dla nagród frakcyjnych (neutrale nietknięte).
    int srcTown, srcTier, srcUpg;
    bool factionCreature = ReverseLookup(origType, srcTown, srcTier, srcUpg);
    if (factionCreature) {
        int newCount = -1;

        // 1) [CountOverride] — twarde wartości per (frakcja, tier, stan banku).
        const int* ov = Config::countOverride[town][srcTier];
        if (ov[0] >= 0 || ov[1] >= 0 || ov[2] >= 0 || ov[3] >= 0) {
            int stateIdx = FindBankStateIdx(origType, count);
            if (stateIdx >= 0 && ov[stateIdx] >= 0) {
                newCount = ov[stateIdx];
                LogDebug("override: stan=%d tier=%d -> count %d -> %d",
                         stateIdx, srcTier + 1, count, newCount);
            } else {
                LogDebug("override: stan nierozpoznany (type=%d count=%d) — pomijam",
                         origType, count);
            }
        }

        // 2) CountMode=1 — przelicz po AI value (gdy nagroda zamieniona).
        // Wprost: newCount = round(count * AI(oryg) / AI(native)), min 1.
        // Zaokrąglenie matematyczne (0.5 w górę). BEZ reguły „min +1" między
        // stanami — silniejszy bank może dać tyle samo co słabszy.
        if (newCount < 0 && Config::countMode == 1 && native >= 0) {
            int aiOld = AiValue(origType);
            int aiNew = AiValue(native);
            if (aiOld > 0 && aiNew > 0) {
                newCount = (count * aiOld + aiNew / 2) / aiNew;   // round half up
                if (newCount < 1) newCount = 1;
                LogDebug("aivalue: %d x %d(AI %d) -> %d x %d(AI %d)",
                         count, origType, aiOld, newCount, native, aiNew);
            }
        }

        // 3) [CountMultiplier] — procentowo, NA wyniku poprzednich kroków.
        if (Config::countPct[town] != 100) {
            int b = (newCount >= 0) ? newCount : count;
            newCount = (b * Config::countPct[town] + 50) / 100;
            LogDebug("count: %d -> %d (pct=%d, town=%d)",
                     b, newCount, Config::countPct[town], town);
        }

        if (newCount >= 0) {
            if (newCount < 1)   newCount = 1;
            if (newCount > 127) newCount = 127;
            if (newCount != count)
                *reinterpret_cast<int8_t*>(bank + 0x58) = static_cast<int8_t>(newCount);
        }
    }
    return EXEC_DEFAULT;
}

// Skan pamięci sygnaturą — zwraca adres dopasowania lub 0.
static uintptr_t ScanSignature(uintptr_t start, size_t size) {
    for (size_t i = 0; i + SIG_LEN <= size; ++i) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(start + i);
        bool ok = true;
        for (int j = 0; j < SIG_LEN; ++j)
            if (SIG_MASK[j] && p[j] != SIG[j]) { ok = false; break; }
        if (ok) return start + i;
    }
    return 0;
}

static DWORD WINAPI StartupPopupThread(LPVOID) {
    // Poczekaj aż gra wczyta genrltxt (żeby popup wyszedł w języku GRY);
    // po ~30 s odpuść — CurLang() da wtedy EN.
    for (int i = 0; i < 300 && TryLang() == 0; ++i) Sleep(100);
    MessageBoxW(nullptr, PopupText(),
        L"HotA - Native Bank Rewards", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
    return 0;
}

static void InstallHook() {
    LogInit();
    if (!Config::Load()) LogDebug("brak INI — używam domyślnych");
    if (!Config::enabled) { LogInfo("plugin wyłączony w INI"); return; }
    SetFactoryT7Dreadnought(Config::factoryT7 == 1);

    // Zakres skanu = obraz głównego modułu (exe). SizeOfImage z nagłówka PE
    // (tylko kernel32 — bez psapi/GetModuleInformation, unikamy zależności
    // K32GetModuleInformation która bywa problematyczna pod Wine).
    HMODULE hExe = GetModuleHandleW(nullptr);
    if (!hExe) { LogError("GetModuleHandle(NULL) == NULL"); return; }
    uintptr_t base = reinterpret_cast<uintptr_t>(hExe);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) { LogError("zły DOS header"); return; }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) { LogError("zły NT header"); return; }
    DWORD imageSize = nt->OptionalHeader.SizeOfImage;

    uintptr_t found = ScanSignature(base, imageSize);
    if (!found) {
        LogError("sygnatura FUN_004abab0 nie znaleziona — inna wersja HotA? Hooki NIE zainstalowane.");
        return;   // fail-safe: zero crashy na nieznanej wersji
    }
    g_moduleBase = found - rva::SIG_ENTRY;
    if (g_moduleBase != base) {
        // Sygnatura powinna leżeć na base+RVA; jeśli nie — ostrzeż ale kontynuuj.
        LogDebug("uwaga: base=0x%p, wyliczony moduleBase=0x%p", (void*)base, (void*)g_moduleBase);
    }

    Patcher* p = GetPatcher();
    if (!p) { LogError("GetPatcher() == NULL (patcher_x86 niedostępny)"); return; }
    PatcherInstance* pi = p->CreateInstance("H3.NativeBanks");
    if (!pi) { LogError("CreateInstance failed"); return; }

    // NH3API: WriteLoHook przyjmuje const void* (H3API brało typowany _LoHookFunc_).
    pi->WriteLoHook(g_moduleBase + rva::HOOK_AT, reinterpret_cast<const void*>(&OnBankReward));
    LogInfo("hook zainstalowany @ 0x%p (moduleBase 0x%p)",
            (void*)(g_moduleBase + rva::HOOK_AT), (void*)g_moduleBase);

    // Popup „mod aktywny" — w osobnym wątku, żeby nie blokować ładowania DLL
    // (MessageBox w DllMain trzymałby loader lock). Jednorazowo przy starcie.
    if (Config::startupPopup)
        CreateThread(nullptr, 0, &StartupPopupThread, nullptr, 0, nullptr);
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH)
        InstallHook();
    return TRUE;
}
