// SPDX-License-Identifier: MIT
#ifndef JSONLIB_CONFIG_HPP
#define JSONLIB_CONFIG_HPP

/* ==========================================================================
 * jsonlib - konfiguracja w JEDNYM pliku.
 *
 * Nic nie trzeba podawac w Makefile ani w linii komend: wystarczy
 * odkomentowac JEDNA linie w kazdej sekcji ponizej i przebudowac.
 * (Cele CI moga nadpisac te wartosci przez -D; -D ma pierwszenstwo,
 *  bo kazda sekcja jest opakowana w #ifndef.)
 * ========================================================================== */

#ifndef JSONLIB_ASSERT
#define JSONLIB_ASSERT(expr) ((void)0)
#endif

/* jsmn zawsze pracuje w trybie strict (gramatyka JSON: liczby, litery,
 * brak kluczy-obiektow). Definiowane tutaj, wiec jsmn.c tez to widzi
 * (src/jsmn.c dolacza ten naglowek) - nie trzeba -DJSMN_STRICT. */
#ifndef JSMN_STRICT
#define JSMN_STRICT 1
#endif

/* --------------------------------------------------------------------------
 * 1. Backend readera. Odkomentuj JEDEN tryb.
 *    0 = tokeny jsmn (szybki dostep losowy, ~2.5 KB RAM na JsonDoc),
 *    1 = skaner span (20 B RAM, dostep sekwencyjny kursorami),
 *    2 = hybryda (tokeny + re-parse spanem przy NOMEM).
 * -------------------------------------------------------------------------- */
#define JSONLIB_READER_TOKEN  0
#define JSONLIB_READER_SPAN   1
#define JSONLIB_READER_HYBRID 2

#ifndef JSONLIB_READER_MODE
// #define JSONLIB_READER_MODE JSONLIB_READER_TOKEN
// #define JSONLIB_READER_MODE JSONLIB_READER_SPAN
#define JSONLIB_READER_MODE JSONLIB_READER_HYBRID
#endif

#if (JSONLIB_READER_MODE < JSONLIB_READER_TOKEN) || \
    (JSONLIB_READER_MODE > JSONLIB_READER_HYBRID)
#error invalid JSONLIB_READER_MODE (wybierz TOKEN / SPAN / HYBRID w config.hpp)
#endif

/* --------------------------------------------------------------------------
 * 2. Glebokosc zagniezdzenia (root object/array liczy sie jako poziom 1).
 *    Ten sam limit obowiazuje reader (token + span) i writer.
 * -------------------------------------------------------------------------- */
#ifndef JSONLIB_JSON_MAX_DEPTH
#define JSONLIB_JSON_MAX_DEPTH 8
// #define JSONLIB_JSON_MAX_DEPTH 6
// #define JSONLIB_JSON_MAX_DEPTH 12
#endif

/* --------------------------------------------------------------------------
 * 3. Pula tokenow (tylko tryby TOKEN i HYBRID).
 *    Jeden token = 8 B RAM w JsonDoc. Liczba tokenow = wartosci + klucze
 *    + obiekty + tablice.
 *
 *    Wymiarowanie dla ramek RX 2-4 KB (cel projektu): typowy JSON to
 *    ~155 tokenow / KB (pomiar na ramkach katalogowych), czyli 2 KB to
 *    ~310 tokenow, 4 KB to ~620. Patologiczne gestosci (same krotkie
 *    tablice, np. [1,1,...]) dochodza do ~500 / KB - takich ramek tokeny
 *    nie kryja, uzyj trybu SPAN albo powieksz cap kosztem 8 B / token.
 * -------------------------------------------------------------------------- */
#if (JSONLIB_READER_MODE == JSONLIB_READER_TOKEN) || \
    (JSONLIB_READER_MODE == JSONLIB_READER_HYBRID)
#ifndef JSONLIB_TOKEN_CAP
#if JSONLIB_READER_MODE == JSONLIB_READER_TOKEN
/* 640 tokenow = 5 KB RAM; kryje 4 KB typowej gestosci w sciezce tokenowej. */
#define JSONLIB_TOKEN_CAP 320
// #define JSONLIB_TOKEN_CAP 256
#else
/* Hybrid: szybka sciezka kryje ramki do ~2.5 KB (3 KB RAM);
 * wieksze (3-4 KB) ida span-fallbackiem: poprawnie, ale drozej. */
#define JSONLIB_TOKEN_CAP 128
// #define JSONLIB_TOKEN_CAP 256
#endif
#endif
#endif

/* --------------------------------------------------------------------------
 * 4. Floaty. 1 = wylacz float/double (oszczedza ~3.5 KB flash: brak
 *    snprintf %g i strtof; uzycie Val(float)/Float() to blad kompilacji).
 * -------------------------------------------------------------------------- */
#ifndef JSONLIB_NO_FLOAT
#define JSONLIB_NO_FLOAT 0
// #define JSONLIB_NO_FLOAT 1
#endif

#endif
