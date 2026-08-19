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
