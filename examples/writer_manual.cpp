// SPDX-License-Identifier: MIT
//
// Writer example: same document as writer_basic.cpp, but the issues array
// is opened and closed with explicit BeginArray()/EndArray() calls on the
// builder instead of an Array() scope.
// Embedded notes:
// - the builder keeps one LIFO stack, so manual Begin/End calls must nest
//   exactly like the scopes they replace,
// - inside an object the array must be preceded by Key() (here "issues"),
// - never mix a manual End* with a still-live scope for the same level:
//   the scope destructor would close it a second time and set ok() to false,
// - always check ok() before handing the buffer to the transport.
#include "jsonlib/json.hpp"

#include <cstdio>

int main()
{
  char tx[256];

  jsonlib::JsonBuilder b(tx, sizeof(tx));
  {
    auto root = b.Object();
    root["kind"] = "report";
    root["id"] = 1U;
    {
      auto data = root["data"].Object();
      data["label"] = "Base";
      data["count"] = 12500;
      data["active"] = false;

      b.Key("issues").BeginArray();
       b.BeginObject();
        b.Pair("code", 3u);
       b.EndObject();
       b.BeginObject().Pair("code", 7).EndObject();
      b.EndArray();
    }
  }

  if (!b.ok())
  {
    std::printf("TX buffer too small\n");
    return 1;
  }

  std::printf("%s\n", b.c_str());
  return 0;
}
