# jsonlib

[Polski](README.pl.md)

**Allocation-free JSON reader/writer for embedded C++17.** Small, strict, and
built on the MIT-licensed [jsmn](https://github.com/zserge/jsmn) tokenizer.

## What it does

- **Reads** JSON from a caller-owned buffer with strict validation: one root
  value, no trailing bytes, full number/literal grammar, no raw control bytes
  in strings, and a container depth cap.
- **Writes** JSON into a fixed buffer: bounded output, automatic escaping,
  and overflow detection.
- **No heap, no exceptions, no RTTI.** All storage is caller-owned or static.

## Who it's for

Firmware on MCUs (Cortex-M, AVR, and similar) and RTOS projects that need JSON
with predictable RAM/flash usage and no dynamic allocation. Aimed at UART/DMA
frames, commands, and telemetry payloads of a few KB.

## Reader

```cpp
#include "jsonlib/json.hpp"

jsonlib::JsonDoc doc;
if (!doc.Parse(rx, rx_len))
{
    report(doc.error()); // your error path; Syntax, Partial, NoMemory, TooDeep
    return;
}

jsonlib::JsonValue root = doc.root();
const int32_t id = root["id"].Int().value_or(0);
const auto kind = root["kind"].Str().value_or(std::string_view{});
```

Accessors return `std::optional`, so missing fields, `null`, and type
mismatches share one defaulting path. Iterate arrays and objects with
`JsonArrayCursor` and `JsonObjectCursor`.

## Writer

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
    return; // buffer too small: send nothing
}
send_frame(tx, b.len());
```

## Examples are the documentation

`examples/` contains complete, self-documenting programs for the advanced
cases:

- **Reader**: `reader_basic.cpp` (typed access and defaults),
  `reader_embedded_rx.cpp` (length-delimited UART/DMA buffer, static token
  pool, `NoMemory`), `reader_iterate.cpp` (allocation-free cursors).
- **Writer**: `writer_basic.cpp` (RAII scopes), `writer_manual.cpp` (explicit
  `BeginArray()`/`EndArray()`), `writer_escape.cpp` (escaping),
  `writer_overflow.cpp` (overflow handling), `writer_cpp_buffers.cpp`
  (`std::array`, `std::vector`, C array), `writer_span.cpp` (C++20
  `std::span`).

```sh
make test            # unit tests + all C++17 examples
make examples-cxx20  # std::span example (C++20)
```

## Configuration

Edit `include/jsonlib/config.hpp`; no build flags are needed.

- Reader backend: `TOKEN` (token pool, fast random access, 8 B/token),
  `SPAN` (20 B state, sequential cursor traversal, no input limit), or
  `HYBRID` (default; token fast path with span fallback).
- `JSONLIB_JSON_MAX_DEPTH` (default 8), `JSONLIB_TOKEN_CAP` (token/hybrid),
  and `JSONLIB_NO_FLOAT` (drops float support, saving ~3.5 KB flash).
- In `TOKEN`/`HYBRID` mode `JsonDoc` contains the token pool; keep it
  `static`, never on the task stack.

## Build and integrate

`make` produces `build/libjsonlib.a`. To integrate manually, add `include/`
and `third_party/jsmn/` to the include path and compile `src/json_doc.cpp`,
`src/json_primitives.cpp`, and `src/json_writer.cpp`, plus the backend sources
selected in `config.hpp`.

## License

MIT. See [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
