// mapping.cpp — implementacja tabel native i lookupów.
#include "mapping.h"

namespace nb {

// Tabela native: [town][tier-1][base=0/upg=1].
// SoD (0-8): deterministyczne z enum eCreatures. Conflux nieciągły.
// HotA (9-11): ID z HotA.dat. Źródło: ../mapping.md.
const int16_t NATIVE[TOWN_COUNT][TIER_COUNT][2] = {
    // Castle
    {{0,1},{2,3},{4,5},{6,7},{8,9},{10,11},{12,13}},
    // Rampart
    {{14,15},{16,17},{18,19},{20,21},{22,23},{24,25},{26,27}},
    // Tower
    {{28,29},{30,31},{32,33},{34,35},{36,37},{38,39},{40,41}},
    // Inferno
    {{42,43},{44,45},{46,47},{48,49},{50,51},{52,53},{54,55}},
    // Necropolis
    {{56,57},{58,59},{60,61},{62,63},{64,65},{66,67},{68,69}},
    // Dungeon
    {{70,71},{72,73},{74,75},{76,77},{78,79},{80,81},{82,83}},
    // Stronghold
    {{84,85},{86,87},{88,89},{90,91},{92,93},{94,95},{96,97}},
    // Fortress
    {{98,99},{100,101},{102,103},{104,105},{106,107},{108,109},{110,111}},
    // Conflux (nieciągłe ID)
    {{118,119},{112,127},{115,123},{114,129},{113,125},{120,121},{130,131}},
    // Cove (HotA)
    {{153,154},{155,156},{157,158},{159,160},{161,162},{163,164},{165,166}},
    // Factory (HotA 1.7); tier1 base = SoD Halfling 138; tier7 = Couatl
    {{138,171},{172,173},{174,175},{176,177},{178,179},{180,181},{182,183}},
    // Bulwark (HotA 1.8)
    {{186,187},{188,189},{190,191},{192,193},{194,195},{196,197},{198,199}},
};

static bool g_factoryDread = false;
void SetFactoryT7Dreadnought(bool d) { g_factoryDread = d; }

bool ReverseLookup(int creatureId, int& town, int& tier, int& upgraded) {
    // Dreadnought/Juggernaut: alternatywny t7 Factory (nie ma go w NATIVE)
    if (creatureId == 184 || creatureId == 185) {
        town = TOWN_FACTORY; tier = 6; upgraded = (creatureId == 185);
        return true;
    }
    for (int t = 0; t < TOWN_COUNT; ++t)
        for (int lvl = 0; lvl < TIER_COUNT; ++lvl)
            for (int u = 0; u < 2; ++u)
                if (NATIVE[t][lvl][u] == creatureId) {
                    town = t; tier = lvl; upgraded = u;
                    return true;
                }
    return false;
}

int RemapReward(int originalCreatureId, int recipientTown) {
    if (recipientTown < 0 || recipientTown >= TOWN_COUNT)
        return -1;
    int srcTown, tier, upg;
    if (!ReverseLookup(originalCreatureId, srcTown, tier, upg))
        return -1;                       // neutral / nierozpoznany -> zostaw
    if (srcTown == recipientTown)
        return -1;                       // już native -> nie ruszaj
    int16_t native = NATIVE[recipientTown][tier][upg];
    if (g_factoryDread && recipientTown == TOWN_FACTORY && tier == 6)
        native = static_cast<int16_t>(upg ? 185 : 184);   // Dreadnought/Juggernaut
    return native;
}

} // namespace nb
