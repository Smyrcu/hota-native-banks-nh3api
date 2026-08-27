# Native Bank Rewards — wariant na NH3API

Plugin do Heroes III: Horn of the Abyss (HD Mod) podmieniający nagrody jednostkowe
z creature banków na jednostki **native frakcji gracza** (tier→tier), z balansem
liczebności po AI value i dialogiem wyboru t7 dla Factory.

To **drugie repo** — ta sama funkcjonalność co bazowy plugin, ale zbudowana na
[NH3API](https://github.com/void2012/NH3API) zamiast H3API. Logika (mapowanie,
config, offsety HotA 1.8.0) jest wspólna; NH3API dostarcza tu wyłącznie patcher
(`GetPatcher`/`HookContext`/`LoHook`) — struktury HotA adresowane są surowo przez
`g_moduleBase + rva::*` (offsety wyRE-owane ręcznie, patrz oryginalne `findings.md`).

## Build

NH3API jest submodułem — po klonowaniu:

```
git submodule update --init
```

Cross-compile na Linuxie (mingw-w64 i686, GCC):

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

NH3API wspiera C++17 na: MSVC 19.14+, MinGW GCC 9+, oraz **llvm-mingw**
(rekomendowany przez autora NH3API, działa cross z Linuksa i macOS).

## Instalacja

Zbudowaną `native_banks.dll` przemianuj na `setseed.dll` i wrzuć do
`...\_HD3_Data\Common\`. Online: wszyscy gracze muszą mieć ten sam plik.

## Licencje

- NH3API (submoduł `third_party/NH3API`): Apache License 2.0 — permisywna,
  nie wymusza upubliczniania kodu korzystającego.
- Licencja tego pluginu: do ustalenia.

## Kontynuacja / kontekst

Pełny status projektu, kontakty, decyzje i backlog: patrz **`docs/STATUS.md`
w repo bazowym `Smyrcu/hota-native-banks`** (wariant H3API). Oba repo dzielą
logikę i offsety; tu różni się tylko biblioteka.

Delta portu względem H3API:
- include: `nh3api/core/nh3api_std/patcher_x86.hpp` zamiast H3API `patcher_x86.hpp`
- `WriteLoHook` bierze `const void*` → `reinterpret_cast<const void*>(&OnBankReward)`
- reszta (main.cpp logika, mapping/config/log) skopiowana 1:1
- build: ten sam podman + mingw i686 (patrz wyżej); llvm-mingw też działa

## Weryfikacja

- 2026-08-27, HotA **1.8.1** (GOG build 59987503113364638), v1.0: potwierdzone w grze — hook @0x004abbfa, remapy Stronghold (Wyvern→Cyklop 1:1, Giant→Behemot, 4×Anioł→6×Behemot po AI value), poprawny no-op dla nagrody już native. Log z `Debug=1` bez błędów.
