// SPDX-License-Identifier: MIT
//
// Writer example: string values are quoted and escaped by the builder.
// This matters for serial numbers, SSIDs and user-provided names that may
// contain quotes, backslashes or control bytes.
#include "jsonlib/json.hpp"

#include <cstdio>
#include <string_view>

int main()
{
  char tx[128];

  jsonlib::JsonBuilder b(tx, sizeof(tx));
  {
    auto root = b.Object();
    root["Text"] = "line1\nline2\t\"quoted\"\\";
    root["ControlByte"] = std::string_view("A\x01Z", 3u);
  }

  if (!b.ok())
  {
    std::printf("TX buffer too small\n");
    return 1;
  }

  std::printf("%s\n", b.c_str());
  return 0;
}
