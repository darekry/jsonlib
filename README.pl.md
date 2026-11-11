# jsonlib

[English](README.md) | Polski

**Bezalokacyjna biblioteka do odczytu i zapisu JSON dla systemów wbudowanych w
C++17.** Mała, rygorystyczna, oparta na tokenizerze
[jsmn](https://github.com/zserge/jsmn) na licencji MIT.

## Co robi

- **Czyta** JSON z bufora dostarczonego przez wywołującego, z rygorystyczną
  walidacją: jedna wartość główna, brak danych po niej, pełna gramatyka liczb
  i literałów, brak surowych bajtów sterujących w łańcuchach oraz limit
  zagnieżdżenia kontenerów.
- **Pisze** JSON do bufora o stałym rozmiarze: ograniczony wynik, automatyczne
  escapowanie i wykrywanie przepełnienia.
- **Bez heapu, wyjątków i RTTI.** Cała pamięć należy do wywołującego lub jest
  statyczna.

## Dla kogo

Dla twórców firmware'u na mikrokontrolery (Cortex-M, AVR i podobne) oraz
projektów RTOS, które potrzebują JSON-a z przewidywalnym zużyciem RAM i flash,
bez dynamicznej alokacji. Zaprojektowana pod ramki UART/DMA, komendy i dane
telemetryczne rzędu kilku KB.

## Odczyt (reader)

```cpp
#include "jsonlib/json.hpp"

jsonlib::JsonDoc doc;
if (!doc.Parse(rx, rx_len))
{
    report(doc.error()); // Twoja obsługa błędu; Syntax, Partial, NoMemory, TooDeep
    return;
}

jsonlib::JsonValue root = doc.root();
const int32_t id = root["id"].Int().value_or(0);
const auto kind = root["kind"].Str().value_or(std::string_view{});
```

Akcesory zwracają `std::optional`, więc brak pola, `null` i niezgodny typ
obsługuje się jednym mechanizmem wartości domyślnych. Do iteracji po tablicach
i obiektach służą `JsonArrayCursor` i `JsonObjectCursor`.

## Zapis (writer)

```cpp
#include "jsonlib/json.hpp"

char tx[256];
jsonlib::JsonBuilder b(tx, sizeof tx);
{
    auto root = b.Object();
    root["kind"] = "report";
    root["id"] = 1U;
    {
        auto data = root["data"].Object();
        data["count"] = 12500;
        data["active"] = false;
    }
}
if (!b.ok())
{
    return; // bufor za mały: nic nie wysyłaj
}
send_frame(tx, b.len());
```

## Przykłady są dokumentacją

Katalog `examples/` zawiera kompletne, samodokumentujące się programy dla
bardziej zaawansowanych przypadków:

- **Reader**: `reader_basic.cpp` (typowy dostęp i wartości domyślne),
  `reader_embedded_rx.cpp` (bufor UART/DMA bez terminatora, statyczna pula
  tokenów, `NoMemory`), `reader_iterate.cpp` (iteracja kursorami bez
  alokacji).
- **Writer**: `writer_basic.cpp` (zakresy RAII), `writer_manual.cpp` (jawne
  `BeginArray()`/`EndArray()`), `writer_escape.cpp` (escapowanie),
  `writer_overflow.cpp` (obsługa przepełnienia), `writer_cpp_buffers.cpp`
  (`std::array`, `std::vector`, tablica C), `writer_span.cpp` (C++20
  `std::span`).

```sh
make test            # testy jednostkowe + wszystkie przykłady C++17
make examples-cxx20  # przykład std::span (C++20)
```

## Konfiguracja

Edytuj `include/jsonlib/config.hpp`; żadne flagi budowania nie są potrzebne.

- Backend readera: `TOKEN` (pula tokenów, szybki dostęp losowy, 8 B/token),
  `SPAN` (20 B stanu, sekwencyjne przechodzenie kursorami, brak limitu
  wejścia) albo `HYBRID` (domyślnie; szybka ścieżka tokenowa z fallbackiem
  span).
- `JSONLIB_JSON_MAX_DEPTH` (domyślnie 8), `JSONLIB_TOKEN_CAP` (token/hybrid)
  oraz `JSONLIB_NO_FLOAT` (wyłącza float, oszczędzając ~3,5 KB flash).
- W trybie `TOKEN`/`HYBRID` `JsonDoc` zawiera pulę tokenów; trzymaj go jako
  `static`, nigdy na stosie zadania.

## Budowanie i integracja

`make` tworzy `build/libjsonlib.a`. Przy ręcznej integracji dodaj `include/`
i `third_party/jsmn/` do ścieżki nagłówków i skompiluj `src/json_doc.cpp`,
`src/json_primitives.cpp` oraz `src/json_writer.cpp`, plus źródła backendu
wybranego w `config.hpp`.

## Licencja

MIT. Zobacz [LICENSE](LICENSE) i [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
