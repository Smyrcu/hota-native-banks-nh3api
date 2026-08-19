// config.h — odczyt native_banks.ini (obok DLL).
#pragma once

namespace Config {
    extern bool enabled;      // czy plugin aktywny
    extern bool debug;        // logowanie debug
    // factionMode: 0 = frakcja gracza/startowe miasto (domyślne), 1 = klasa bohatera
    extern int  factionMode;
    // countMode: 0 = zachowaj oryginalną liczbę, 1 = przelicz po AI value
    // (liczba = round(orygLiczba * AI(oryginał) / AI(native)))
    extern int  countMode;
    // factoryT7: 0 = Couatl (182/183), 1 = Dreadnought (184/185)
    extern int  factoryT7;
    // startupPopup: 1 = pokaż okno „mod aktywny" przy starcie gry, 0 = nie
    extern int  startupPopup;
    // language: język komunikatów (popup + dialog Factory).
    // 0 = auto (wykrywany z tekstów GRY, fallback EN), 1 = pl, 2 = en, 3 = ru.
    // INI przyjmuje też stringi: auto/pl/en/ru.
    extern int  language;
    // Mnożnik liczby jednostek nagrody per frakcja odbiorcy, w procentach
    // (100 = bez zmian). Indeks = town 0..11. Sekcja INI [CountMultiplier].
    extern int  countPct[12];
    // Twarde nadpisanie liczby jednostek per (frakcja, tier, stan banku 0..3).
    // -1 = brak nadpisania. Sekcja INI [CountOverride], klucze "<Town>.Tier<1-7>"
    // z 4 wartościami po przecinku (stany banku od najsłabszego).
    extern int  countOverride[12][7][4];

    bool Load();              // true jeśli INI znaleziony i wczytany
}
