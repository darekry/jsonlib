// SPDX-License-Identifier: MIT
//
// Writer example (C++20): std::span is a non-owning view, exactly like the
// raw pointer constructor. The pointed-to storage must outlive the builder.
#include "jsonlib/json.hpp"

#include <cstdio>
#include <span>

int main()
{
  char tx[128];

  jsonlib::JsonBuilder b{std::span<char>(tx)};
  {
    auto root = b.Object();
    root["Kind"] = "span";
    root["Value"] = 45;
  }

  if (!b.ok())
  {
    std::printf("TX buffer too small\n");
    return 1;
  }

  std::printf("span: %s\n", b.c_str());
  return 0;
}
