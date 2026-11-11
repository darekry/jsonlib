# Third-party notices

## jsmn

`third_party/jsmn/jsmn.h` is the upstream jsmn tokenizer by Serge Zaitsev,
locally modified to use compact 16-bit token fields (`jsmntok_t` is 8 bytes on
32-bit targets; token-mode inputs are limited to 32767 bytes) and an
open-container counter in `jsmn_parser` that replaces the final token-pool scan.

- License: MIT
- Copyright (c) 2010 Serge Zaitsev
- The full license text is retained at the top of `third_party/jsmn/jsmn.h`.

```text
MIT License

Copyright (c) 2010 Serge Zaitsev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

The rest of `jsonlib` is independent of the upstream project and only uses the
public jsmn API (`jsmn_init`, `jsmn_parse`, `jsmntok_t`).
