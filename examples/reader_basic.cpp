// SPDX-License-Identifier: MIT
//
// Reader example: typed access with optional values and defaults.
// Embedded notes:
// - the token array is static (no heap, no std::vector),
// - string values are string_view slices into the RX buffer (no copies),
// - missing / null / wrong-type fields are handled with optional<>.
#include "jsonlib/json.hpp"

#include <cstdio>
#include <string_view>

namespace {

constexpr char kFrame[] =
  R"({"id":1,"kind":"widget","label":"Base","count":2500,"active":true,"details":{"target":12.5,"tags":[true,null,"x"]}})";

jsonlib::Token g_tokens[48];

}

int main()
{
  jsonlib::JsonParser parser;
  if (!parser.Parse(kFrame, sizeof(kFrame) - 1u, g_tokens, 48))
  {
    std::printf("parse failed: %d\n", (int)parser.error());
    return 1;
  }

  jsonlib::JsonView v = parser.View();

  auto label = v["label"].Str();
  if (!label)
  {
    std::printf("label missing or not a string\n");
    return 2;
  }
///
  const int32_t count = v["count"].Int().value_or(1000); 

  const bool active = v["active"].Bool().value_or(false);
  auto details = v["details"];

  const float target = details["target"].Float().value_or(0.0f);
  const bool first_tag = v["details"]["tags"][0u].Bool().value_or(false);

  const auto kind = v["kind"].Str().value_or(std::string_view());
  std::printf("kind=%.*s label=%.*s count=%d active=%d target=%.2f "
              "tag0=%d\n",
              (int)kind.size(), kind.data(),
              (int)label->size(), label->data(), (int)count,
              active ? 1 : 0, (double)target, first_tag ? 1 : 0);
  return 0;
}
