// SPDX-License-Identifier: MIT
//
// Reader example: iterate over an array of record objects without
// allocation. Both iteration helpers return a token index, so the caller
// decides which fields are copied or applied to the device state.
#include "jsonlib/json.hpp"

#include <cstdio>
#include <string_view>

namespace {

constexpr char kFrame[] =
  R"({"entries":[{"name":"alpha","value":1200},{"name":"beta","value":500},{"name":"gamma","value":"Z"},{"name":"delta","value":null}]})";

jsonlib::Token g_tokens[64];

}

int main()
{
  jsonlib::JsonParser parser;
  if (!parser.Parse(kFrame, sizeof(kFrame) - 1u, g_tokens, 64))
  {
    std::printf("parse failed: %d\n", (int)parser.error());
    return 1;
  }

  jsonlib::JsonView v = parser.View();
 auto  entries = v["entries"];
  if (!entries.IsArray())
  {
    std::printf("entries is not an array\n");
    return 2;
  }

  size_t numbers = 0u;
  size_t strings = 0u;
  size_t skipped = 0u;

  jsonlib::JsonForEachElement(v, entries.Tok(),
                              [&](int elem_tok)
                              {
                                jsonlib::JsonNode item = v.At(elem_tok);
                                if (!item.IsObject())
                                {
                                  skipped++;
                                  return true;
                                }

                                jsonlib::JsonForEachPair(v, item.Tok(),
                                                         [&](std::string_view key,
                                                             int val_tok)
                                                         {
                                                           jsonlib::JsonNode value =
                                                             v.At(val_tok);
                                                           if (key != "value")
                                                           {
                                                             return true;
                                                           }
                                                           if (!value.Exists() ||
                                                               value.IsNull())
                                                           {
                                                             skipped++;
                                                             return true;
                                                           }
                                                           if (value.Int())
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
                                                           return true;
                                                         });
                                return true;
                              });

  std::printf("numbers=%zu strings=%zu skipped=%zu\n", numbers, strings,
              skipped);
  return 0;
}
