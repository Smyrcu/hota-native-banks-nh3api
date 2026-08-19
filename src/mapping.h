// mapping.h — tabele native tier→creature per frakcja + odwrotny lookup.
// Dane zweryfikowane z H3API (SoD) i HotA.dat (Cove/Factory/Bulwark).
// Szczegóły i źródła: ../mapping.md, ../findings.md
#pragma once
#include <cstdint>

namespace nb {

// Liczba frakcji (11 w HotA 1.8) i tierów.
constexpr int FACTION_COUNT = 11;   // 0..8 SoD, 9 Cove, 10 Factory, 11 Bulwark? -> patrz TOWN_*
constexpr int TIER_COUNT    = 7;

// Indeksy miast (town = hero_class/2). Zgodne z kolejnością SoD + HotA.
enum Town {
    TOWN_CASTLE = 0, TOWN_RAMPART, TOWN_TOWER, TOWN_INFERNO, TOWN_NECROPOLIS,
    TOWN_DUNGEON, TOWN_STRONGHOLD, TOWN_FORTRESS, TOWN_CONFLUX,
    TOWN_COVE, TOWN_FACTORY, TOWN_BULWARK,
    TOWN_INVALID = -1
};
constexpr int TOWN_COUNT = 12;

// nativeTable[town][tier][upgraded] = creature ID (-1 = brak/nie mapuj).
extern const int16_t NATIVE[TOWN_COUNT][TIER_COUNT][2];

// Odwrotny lookup: creatureId -> (town, tier, upgraded). Zwraca false jeśli
// stworek nie należy do żadnego rosteru miasta (neutral -> nie mapować).
bool ReverseLookup(int creatureId, int& town, int& tier, int& upgraded);

// Przełącznik t7 Factory: false = Couatl (182/183), true = Dreadnought (184/185).
void SetFactoryT7Dreadnought(bool dreadnought);

// Główna funkcja remapu: dla oryginalnego typu nagrody i miasta odbiorcy
// zwraca native creature ID, albo -1 gdy nie należy mapować.
int RemapReward(int originalCreatureId, int recipientTown);

} // namespace nb
