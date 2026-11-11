// SPDX-License-Identifier: MIT
#include "jsonlib/json.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

namespace {

void TestReader()
{
  static constexpr char kJson[] =
    R"({"name":"Widget","id":1,"details":{"label":"A1","weight":12.5,"host":"10.0.0.1","tags":[true,null,"x"],"note":null}})";

  jsonlib::Token tokens[64];
  jsonlib::JsonParser parser;
  assert(parser.Parse(kJson, sizeof(kJson) - 1u, tokens, 64));
  assert(parser.ok());
  assert(parser.TokenCount() > 0);
  assert(parser.error() == jsonlib::JsonParseError::None);

  jsonlib::JsonView v = parser.View();
  assert(v.valid());
  assert(v["name"].Str().value() == "Widget");
  assert(v["id"].Int().value() == 1);
  assert(v["name"] == "Widget");
  assert(v["name"] != "Gadget");
  assert(v["details"]["label"].Str().value() == "A1");
  assert(v["details"]["weight"].Float().value() == 12.5f);
  assert(v["details"]["tags"][0u].Bool().value());
  assert(v["details"]["tags"][1u].IsNull());
  assert(!v["details"]["tags"][1u]);
  assert(v["details"]["tags"][2u].Str().value() == "x");
  assert(v["details"]["note"].IsNull());
  assert(!v["details"]["note"]);
  assert(!v["details"]["missing"].Exists());
  assert(v["details"].Count() == 5u);

  uint8_t ip[4] = { 0u, 0u, 0u, 0u };
  assert(v["details"]["host"].Ip(ip));
  assert((ip[0] == 10u) && (ip[1] == 0u) && (ip[2] == 0u) && (ip[3] == 1u));

  size_t pairs = 0u;
  jsonlib::JsonForEachPair(v, v["details"].Tok(),
                           [&](std::string_view, int)
                           {
                             pairs++;
                             return true;
                           });
  assert(pairs == 5u);

  size_t elements = 0u;
  jsonlib::JsonForEachElement(v, v["details"]["tags"].Tok(),
                              [&](int)
                              {
                                elements++;
                                return true;
                              });
  assert(elements == 3u);
}

void TestParserErrors()
{
  jsonlib::Token one[1];
  jsonlib::JsonParser parser;
  assert(!parser.Parse("{\"a\":1}", 7u, one, 1));
  assert(parser.error() == jsonlib::JsonParseError::NoMemory);

  jsonlib::Token many[16];
  assert(parser.Parse("{\"a\":1}", 7u, many, 16));
  assert(parser.TokenCount() == 3);
}

void TestPrimitives()
{
  int32_t i = 0;
  assert(jsonlib::JsonParseInt("123", i) && (i == 123));
  assert(!jsonlib::JsonParseInt("12x", i));

  float f = 0.0f;
  assert(jsonlib::JsonParseFloat("1.25", f) && (f == 1.25f));
  assert(!jsonlib::JsonParseFloat("", f));

  bool b = false;
  assert(jsonlib::JsonParseBool("true", b) && b);
  assert(jsonlib::JsonParseBool("false", b) && !b);
  assert(!jsonlib::JsonParseBool("1", b));

  uint8_t ip[4] = { 0u, 0u, 0u, 0u };
  assert(jsonlib::JsonParseIp("192.168.0.1", ip));
  assert((ip[0] == 192u) && (ip[1] == 168u) && (ip[2] == 0u) && (ip[3] == 1u));
  assert(!jsonlib::JsonParseIp("192.168.0", ip));

  char dst[4];
  jsonlib::JsonCopyStr(dst, sizeof(dst), "abcdef");
  assert(strcmp(dst, "abc") == 0);
}

void TestWriter()
{
  char buf[256];
  jsonlib::JsonBuilder b(buf, sizeof(buf));
  {
    auto root = b.Object();
    root["title"] = "Report";
    root["revision"] = 1;
    root["active"] = true;
    root["ratio"] = 1.5f;
    root["extra"].Null();
    {
      auto items = root["items"].Array();
      {
        auto item = items.Object();
        item["name"] = "A";
      }
      {
        auto item = items.Object();
        item["name"] = "B";
      }
    }
    {
      auto meta = root["meta"].Object();
      meta["x"] = 2;
    }
  }

  assert(b.ok());
  assert(strcmp(b.c_str(),
                "{\"title\":\"Report\",\"revision\":1,\"active\":true,"
                "\"ratio\":1.5,\"extra\":null,\"items\":[{\"name\":\"A\"},"
                "{\"name\":\"B\"}],\"meta\":{\"x\":2}}") == 0);
}

void TestWriterEscaping()
{
  char buf[64];
  jsonlib::JsonBuilder b(buf, sizeof(buf));
  b.BeginObject();
  b["S"] = std::string_view("a\"b\n");
  b.EndObject();
  assert(b.ok());
  assert(strcmp(b.c_str(), "{\"S\":\"a\\\"b\\n\"}") == 0);
}

void TestWriterOverflow()
{
  char buf[4];
  jsonlib::JsonBuilder b(buf, sizeof(buf));
  b.BeginObject();
  b["Key"] = "value";
  b.EndObject();
  assert(!b.ok());
}

void TestCppBuffers()
{
  std::array<char, 64> arr{};
  jsonlib::JsonBuilder ab(arr);
  ab.BeginObject();
  ab["Kind"] = "array";
  ab.EndObject();
  assert(ab.ok());
  assert(strcmp(arr.data(), "{\"Kind\":\"array\"}") == 0);

  std::vector<char> vec(64u);
  jsonlib::JsonBuilder vb(vec);
  vb.BeginObject();
  vb["Kind"] = "vector";
  vb.EndObject();
  assert(vb.ok());
  assert(strcmp(vec.data(), "{\"Kind\":\"vector\"}") == 0);

  char raw[64];
  jsonlib::JsonBuilder rb(raw);
  rb.BeginObject();
  rb["Kind"] = "c-array";
  rb.EndObject();
  assert(rb.ok());
  assert(strcmp(raw, "{\"Kind\":\"c-array\"}") == 0);
}

}

int main()
{
  TestReader();
  TestParserErrors();
  TestPrimitives();
  TestWriter();
  TestWriterEscaping();
  TestWriterOverflow();
  TestCppBuffers();
  return 0;
}
