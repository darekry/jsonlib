// SPDX-License-Identifier: MIT
//
// Writer example: a bounded buffer can fail. The builder stops writing,
// ok() becomes false, and the partial buffer must not be sent. This example
// intentionally uses a buffer that is too small.
#include "jsonlib/json.hpp"

#include <cstdio>

int main()
{
  char tx[32];
  // char tx[64];  

  jsonlib::JsonBuilder b(tx, sizeof(tx));
  b.BeginObject();
  b["kind"] = "report";
  b["label"] = "Base";
  b.EndObject();

  if (!b.ok())
  {
    std::printf("frame dropped: capacity=%zu partial_len=%zu\n", sizeof(tx),
                b.len());
    return 0;
  }

  std::printf("unexpected success: %s\n", b.c_str());
  return 1;
}
