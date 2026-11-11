// SPDX-License-Identifier: MIT
#include "jsonlib/json.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace {

using jsonlib::JsonArrayCursor;
using jsonlib::JsonBackend;
using jsonlib::JsonDoc;
using jsonlib::JsonObjectCursor;
using jsonlib::JsonReadError;
using jsonlib::JsonValue;

static_assert(sizeof(jsonlib::Token) == 8u,
              "compact jsmn token expected (JSMN_PARENT_LINKS unsupported)");

void TestReader()
{
  static constexpr char kJson[] =
    R"({"title":"Widget","id":1,"details":{"label":"A1","weight":12.5,"host":"10.0.0.1","tags":[true,null,"x"],"note":null}})";

  jsonlib::Token tokens[64];
  jsonlib::JsonParser parser;
  assert(parser.Parse(kJson, sizeof(kJson) - 1u, tokens, 64));
  assert(parser.ok());
  assert(parser.TokenCount() > 0);
  assert(parser.error() == jsonlib::JsonParseError::None);

  jsonlib::JsonView v = parser.View();
  assert(v.valid());
  assert(v["title"].Str().value() == "Widget");
  assert(v["id"].Int().value() == 1);
  assert(v["title"] == "Widget");
  assert(v["title"] != "Gadget");
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

void TestDocReader()
{
  static constexpr char kJson[] =
    R"({"title":"Widget","id":1,"details":{"label":"A1","weight":12.5,"host":"10.0.0.1","tags":[true,null,"x"],"note":null}})";

  JsonDoc doc;
  assert(doc.Parse(kJson, sizeof(kJson) - 1u));
  assert(doc.error() == JsonReadError::None);
  assert(doc.backend() != JsonBackend::None);
#if JSONLIB_READER_MODE == JSONLIB_READER_SPAN
  assert(doc.backend() == JsonBackend::Span);
  assert(doc.token_count() == 0);
#else
  assert(doc.backend() == JsonBackend::TokenArray);
  assert(doc.token_count() > 0);
#endif

  JsonValue v = doc.root();
  assert(v.IsObject());
  assert(v["title"].Str().value() == "Widget");
  assert(v["id"].Int().value() == 1);
  assert(v["title"] == "Widget");
  assert(v["title"] != "Gadget");
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
  assert(v["details"]["tags"].Count() == 3u);
  assert(v["details"]["label"].Count() == 0u);

  uint8_t ip[4] = { 0u, 0u, 0u, 0u };
  assert(v["details"]["host"].Ip(ip));
  assert((ip[0] == 10u) && (ip[1] == 0u) && (ip[2] == 0u) && (ip[3] == 1u));

  size_t pairs = 0u;
  JsonObjectCursor obj(v["details"]);
  std::string_view key;
  JsonValue value;
  while (obj.Next(key, value))
  {
    pairs++;
  }
  assert(pairs == 5u);

  size_t elements = 0u;
  JsonArrayCursor arr(v["details"]["tags"]);
  while (arr.Next(value))
  {
    elements++;
  }
  assert(elements == 3u);

  JsonDoc second;
  assert(second.Parse("{\"a\":[]}", 8u));
  assert(second.root()["a"].IsArray());
  assert(second.root()["a"].Count() == 0u);
}

void TestDocRawSpans()
{
  static constexpr char kJson[] =
    R"({"S":"a\"b","O":{"K":1},"A":[1,2],"P":-12.5e+2})";

  JsonDoc doc;
  assert(doc.Parse(kJson, sizeof(kJson) - 1u));
  JsonValue v = doc.root();
  assert(v["S"].Raw() == "a\\\"b");
  assert(v["S"].Str().value() == "a\\\"b");
  assert(v["O"].Raw() == "{\"K\":1}");
  assert(v["A"].Raw() == "[1,2]");
  assert(v["P"].Raw() == "-12.5e+2");
  assert(v["P"].Float().value() == -1250.0f);
}

void TestDocDuplicateKeysFirstWins()
{
  static constexpr char kJson[] = R"({"a":1,"a":2})";
  JsonDoc doc;
  assert(doc.Parse(kJson, sizeof(kJson) - 1u));
  assert(doc.root()["a"].Int().value() == 1);

  size_t pairs = 0u;
  JsonObjectCursor obj(doc.root());
  std::string_view key;
  JsonValue value;
  while (obj.Next(key, value))
  {
    pairs++;
  }
  assert(pairs == 2u);
}

void TestDocAccepts()
{
  static constexpr const char *kOk[] = {
    "{}",
    "[]",
    R"({"a":1})",
    R"({"a":0,"b":-0,"c":10,"d":0.5,"e":1e10,"f":-1.5E-2})",
    R"({"a":true,"b":false,"c":null})",
    R"({"a":"x\n\u0041\"\\"})",
    " \t\r\n{\"a\" : [ 1 , 2 ] } \n",
    R"([[[[[[[1]]]]]]])",
    "\"abc\"",
    "42 ",
    "null ",
  };

  for (const char *text : kOk)
  {
    JsonDoc doc;
    assert(doc.Parse(text, strlen(text)));
    assert(doc.error() == JsonReadError::None);
  }

  static constexpr char kRoot[] = "\"abc\"";
  JsonDoc root_doc;
  assert(root_doc.Parse(kRoot, sizeof(kRoot) - 1u));
  assert(root_doc.root().Str().value() == "abc");

  static constexpr char kNumber[] = "42 ";
  JsonDoc num_doc;
  assert(num_doc.Parse(kNumber, sizeof(kNumber) - 1u));
  assert(num_doc.root().Int().value() == 42);

  static constexpr char kNull[] = "null ";
  JsonDoc null_doc;
  assert(null_doc.Parse(kNull, sizeof(kNull) - 1u));
  assert(null_doc.root().IsNull());
}

void TestDocRejects()
{
  static constexpr const char *kBad[] = {
    "",
    "   ",
    "{",
    R"({"a":1)",
    R"({"a":1} trailing)",
    R"({"a":1}{"b":2})",
    R"({"a":tru})",
    R"({"a":1x})",
    R"({"a":+1})",
    R"({"a":.5})",
    R"({"a":01})",
    R"({"a":-01})",
    R"({"a":1.})",
    R"({"a":1e})",
    R"({"a":NaN})",
    R"({"a":"\q"})",
    "{\"a\":\"\\u12\"}",
    "{\"a\":\"x\ny\"}",
    "{\"a\":\"unterminated}",
    "42",
    "[[[[[[[[[1]]]]]]]]]",
  };

  for (const char *text : kBad)
  {
    JsonDoc doc;
    assert(!doc.Parse(text, strlen(text)));
    assert(doc.error() != JsonReadError::None);
    assert(doc.error() != JsonReadError::NoMemory);
  }
}

void TestHybridFallback()
{
  std::string big = R"({"kind":"catalog","entries":[)";
  for (int i = 0; i < 100; i++)
  {
    if (i != 0)
    {
      big += ',';
    }
    big += R"({"name":"item","group":"general","count":60})";
  }
  big += "]}";

  JsonDoc doc;
  const bool parsed = doc.Parse(big.data(), big.size());
#if JSONLIB_READER_MODE == JSONLIB_READER_TOKEN
  assert(!parsed);
  assert(doc.error() == JsonReadError::NoMemory);
#else
  assert(parsed);
  assert(doc.backend() == JsonBackend::Span);
  JsonValue root = doc.root();
  assert(root["kind"].Str().value() == "catalog");
  assert(root["entries"].IsArray());
  assert(root["entries"].Count() == 100u);
  JsonValue last = root["entries"][99u];
  assert(last["name"].Str().value() == "item");
  assert(last["count"].Int().value() == 60);
#endif
}

void TestHybridTokenCap()
{
  std::string medium = R"({"kind":"catalog","entries":[)";
  for (int i = 0; i < 16; i++)
  {
    if (i != 0)
    {
      medium += ',';
    }
    medium += R"({"name":"item","group":"general","count":60})";
  }
  medium += "]}";

  JsonDoc doc;
  assert(doc.Parse(medium.data(), medium.size()));
#if JSONLIB_READER_MODE == JSONLIB_READER_SPAN
  assert(doc.backend() == JsonBackend::Span);
  assert(doc.token_count() == 0);
#else
  assert(doc.backend() == JsonBackend::TokenArray);
  assert(doc.token_count() > 64);
  assert(doc.token_count() <= JSONLIB_TOKEN_CAP);
#endif
  assert(doc.root()["entries"].Count() == 16u);
}

void TestTokenInputLimit()
{
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  std::string at_limit(32767u, ' ');
  at_limit[0] = '{';
  at_limit[1] = '}';
  JsonDoc ok;
  assert(ok.Parse(at_limit.data(), at_limit.size()));

  std::string too_long(32768u, ' ');
  too_long[0] = '{';
  too_long[1] = '}';
  JsonDoc bad;
  assert(!bad.Parse(too_long.data(), too_long.size()));
  assert(bad.error() == JsonReadError::Syntax);
#endif
}

void TestDocErrorKinds()
{
  JsonDoc doc;
  assert(!doc.Parse(R"({"a":tru})", 9u));
  assert(doc.error() == JsonReadError::Syntax);

  assert(!doc.Parse(R"({"a":1)", 6u));
  assert(doc.error() == JsonReadError::Partial);

  assert(!doc.Parse("42", 2u));
  assert(doc.error() == JsonReadError::Partial);
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
  TestDocReader();
  TestDocRawSpans();
  TestDocDuplicateKeysFirstWins();
  TestDocAccepts();
  TestDocRejects();
  TestHybridFallback();
  TestHybridTokenCap();
  TestTokenInputLimit();
  TestDocErrorKinds();
  TestWriter();
  TestWriterEscaping();
  TestWriterOverflow();
  TestCppBuffers();
  return 0;
}
