// SPDX-License-Identifier: MIT
//
// Reader example: a UART/DMA receive buffer is parsed without copying or
// null-terminating it. JsonDoc keeps a static token pool sized for the
// worst-case frame; if it is too small, Parse reports NoMemory (token mode)
// or falls back to the span scanner (hybrid) instead of writing past the pool.
#include "jsonlib/json.hpp"

#include <cstdio>
#include <cstring>

namespace {

constexpr size_t kRxCap = 128u;

uint8_t g_rx[kRxCap];

}

int main()
{
  static constexpr char kFrame[] =
    R"({"status":"ready","id":7,"payload":{"temperature":42,"ready":true,"note":null}})";
  const size_t frame_len = sizeof(kFrame) - 1u;

  std::memcpy(g_rx, kFrame, frame_len);

  jsonlib::JsonDoc doc;
  if (!doc.Parse((const char *)g_rx, frame_len))
  {
    if (doc.error() == jsonlib::JsonReadError::NoMemory)
    {
      std::printf("token pool too small\n");
    }
    else
    {
      std::printf("invalid frame: %d\n", (int)doc.error());
    }
    return 1;
  }

  jsonlib::JsonValue v = doc.root();

  char status[24];
  jsonlib::JsonCopyStr(status, sizeof(status), v["status"].Str().value_or(""));

  const int id = v["id"].Int().value_or(-1);
  const int temperature = v["payload"]["temperature"].Int().value_or(-1);
  const bool ready = v["payload"]["ready"].Bool().value_or(false);
  const bool note_is_null = v["payload"]["note"].IsNull();

  std::printf("tokens=%d status=%s id=%d temperature=%d ready=%d note_null=%d\n",
              doc.token_count(), status, id, temperature, ready ? 1 : 0,
              note_is_null ? 1 : 0);
  return 0;
}
