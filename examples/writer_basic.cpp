// SPDX-License-Identifier: MIT
//
// Writer example: build a nested response into a fixed TX buffer.
// Embedded notes:
// - the builder never allocates; the caller supplies the buffer,
// - RAII object/array scopes close JSON automatically,
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
      {
        auto issues = data["issues"].Array();
        issues.Object()["code"] = 3;
        issues.Object()["code"] = 7;
      }
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
