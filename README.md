# jsonlib

Small, allocation-free C++17 JSON reader/writer built on the MIT-licensed
[jsmn](https://github.com/zserge/jsmn) tokenizer. The library is standalone: it
does not depend on the firmware, HAL, or RTOS.

## Scope

- `jsonlib::JsonParser` - non-owning parser over a caller-supplied byte buffer
  and token array.
- `jsonlib::JsonView` / `jsonlib::JsonNode` - typed, chainable read access:
  - `v["details"]["id"].Int()`
  - `v["items"][0u].Str()`
  - `Exists()`, `IsNull()`, `IsObject()`, `IsArray()`, `IsString()`,
    `IsPrimitive()`, `Count()`
  - `Str()`, `Int()`, `Float()`, `Bool()`, `Ip()`
  - `==` / `!=` against a string value
- `jsonlib::JsonBuilder` - bounded, zero-allocation JSON writer:
  - `b["Key"] = value`, `.Null()`, `.Object()`, `.Array()`
  - RAII object/array scopes
  - constructors for `char * + size`, C arrays, `std::array<char, N>`,
    `std::vector<char>` and `std::span<char>` (C++20)
- Primitive helpers: `JsonParseInt`, `JsonParseFloat`, `JsonParseBool`,
  `JsonParseIp`, `JsonCopyStr`
- Iteration helpers: `JsonForEachPair`, `JsonForEachElement`

## Properties

- C++17, no exceptions, no RTTI, no heap allocation.
- Caller owns the JSON buffer and the `jsonlib::Token` array.
- Token storage size is bounded by `jsmn` semantics: one token per JSON value,
  key, object and array.
- The library does not impose a maximum document size.
- Integer JSON values are 32-bit. Integer literals are accepted on all
  targets (including ARM, where `int32_t` is `long`); wider types require an
  explicit cast.

## Buffer adapters

`JsonBuilder` only borrows storage; it never owns, resizes or reallocates the
buffer. All adapters have the same lifetime rule: the buffer must outlive the
builder and must not be resized or moved while the builder (or its output) is
in use.

- `JsonBuilder(char *buf, size_t cap)` - explicit pointer and capacity.
- `JsonBuilder(char (&buf)[N])` - C array; capacity is `N`.
- `JsonBuilder(std::array<char, N> &buf)` - capacity is `N`.
- `JsonBuilder(std::vector<char> &buf)` - capacity is `buf.size()`; call
  `resize()` before constructing the builder. The builder does not shrink or
  grow the vector; use `b.len()` for the JSON length.
- `JsonBuilder(std::span<char> buf)` (C++20) - non-owning view; same ownership
  rules as the raw pointer constructor. A temporary span such as
  `JsonBuilder b{std::span<char>(tx)}` is fine as long as `tx` outlives `b`.

## Examples

Embedded-oriented examples are in `examples/`:

Reader:
- `reader_basic.cpp` - typed access, optional values, defaults, nested
  objects and array indexing.
- `reader_embedded_rx.cpp` - length-delimited UART/DMA buffer without a null
  terminator, static token pool, `NoMemory` handling.
- `reader_iterate.cpp` - allocation-free iteration over arrays of records and
  objects, including `null` values.

Writer:
- `writer_basic.cpp` - nested response object with RAII object/array scopes.
- `writer_manual.cpp` - the same document written with explicit
  `BeginArray()`/`EndArray()` calls on the builder instead of an array scope.
- `writer_escape.cpp` - quoting and escaping of strings and control bytes.
- `writer_overflow.cpp` - bounded TX buffer failure; partial output must not
  be sent.
- `writer_cpp_buffers.cpp` - `std::array`, pre-sized `std::vector` and C array
  adapters.
- `writer_span.cpp` (C++20) - temporary `std::span<char>` adapter.

Build and run them with:

```sh
make -C libs/jsonlib examples              # C++17 examples
make -C libs/jsonlib check-examples
make -C libs/jsonlib examples-cxx20        # std::span example
make -C libs/jsonlib check-examples-cxx20
```

`make -C libs/jsonlib test` builds and runs the unit test and all C++17
examples. The span example requires a C++20 compiler. The examples use
`printf` only for host-side demonstration; in firmware replace it with the
platform log/UART output.

## Integration

Add the include paths and compile the library sources:

```sh
g++ -std=c++17 -Ilibs/jsonlib/include -Ilibs/jsonlib/third_party/jsmn \
    -c libs/jsonlib/src/json_reader.cpp
g++ -std=c++17 -Ilibs/jsonlib/include -Ilibs/jsonlib/third_party/jsmn \
    -c libs/jsonlib/src/json_writer.cpp
gcc -std=c17 -Ilibs/jsonlib/third_party/jsmn -c libs/jsonlib/src/jsmn.c
```

Or build the static library and run the standalone test:

```sh
make -C libs/jsonlib
make -C libs/jsonlib test
```

Set `JSONLIB_ASSERT(expr)` before including the library headers if assertions
are required. The default is a no-op, matching embedded release builds.

## License

`jsonlib` is released under the MIT license (see `LICENSE`). The bundled jsmn
header keeps its own MIT license (see `THIRD_PARTY_NOTICES.md`). MIT permits
use in closed-source/proprietary products.
