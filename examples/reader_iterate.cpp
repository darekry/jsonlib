// SPDX-License-Identifier: MIT
//
// Reader example: iterate over an array of record objects without
// allocation. Cursors walk the document sequentially, which is the only
// efficient access pattern in span mode (indexed access re-scans).
#include "jsonlib/json.hpp"

#include <cstdio>
#include <string_view>

namespace {

constexpr char kFrame[] =
  R"({"entries":[{"name":"alpha","value":1200},{"name":"beta","value":500},{"name":"gamma","value":"Z"},{"name":"delta","value":null}]})";

}

int main()
{
  jsonlib::JsonDoc doc;
  if (!doc.Parse(kFrame, sizeof(kFrame) - 1u))
  {
    std::printf("parse failed: %d\n", (int)doc.error());
    return 1;
  }

  jsonlib::JsonValue entries = doc.root()["entries"];
  if (!entries.IsArray())
  {
    std::printf("entries is not an array\n");
    return 2;
  }

  size_t numbers = 0u;
  size_t strings = 0u;
  size_t skipped = 0u;

  jsonlib::JsonArrayCursor array(entries);
  jsonlib::JsonValue item;
  while (array.Next(item))
  {
    if (!item.IsObject())
    {
      skipped++;
      continue;
    }

    jsonlib::JsonObjectCursor members(item);
    std::string_view key;
    jsonlib::JsonValue value;
    while (members.Next(key, value))
    {
      if (key != "value")
      {
        continue;
      }
      if (!value.Exists() || value.IsNull())
      {
        skipped++;
      }
      else if (value.Int())
      {
        numbers++;
      }
      else if (value.Str())
      {
        strings++;
      }
      else
      {
        skipped++;
      }
    }
  }

  std::printf("numbers=%zu strings=%zu skipped=%zu\n", numbers, strings,
              skipped);
  return 0;
}
