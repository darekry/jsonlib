// SPDX-License-Identifier: MIT
#ifndef JSONLIB_JSON_READER_HPP
#define JSONLIB_JSON_READER_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#ifndef JSMN_HEADER
#define JSMN_HEADER
#define JSONLIB_UNDEF_JSMN_HEADER
#endif
#include "jsmn.h"
#ifdef JSONLIB_UNDEF_JSMN_HEADER
#undef JSMN_HEADER
#undef JSONLIB_UNDEF_JSMN_HEADER
#endif

namespace jsonlib {

using Token = jsmntok_t;

class JsonNode;

class JsonView
{
public:
  JsonView() = default;
  JsonView(const char *buf, Token *toks, int n);

  int Find(int obj, std::string_view key) const;

  std::string_view View(int tok) const;

  int Type(int tok) const;
  const Token &Tok(int i) const;

  int Skip(int tok) const;

  bool valid() const;
  bool Contains(int tok) const;

  JsonNode At(int tok) const;
  JsonNode operator[](std::string_view key) const;

private:
  const char *buf_ = nullptr;
  Token *toks_ = nullptr;
  int n_ = 0;
};

class JsonNode
{
public:
  JsonNode() = default;
  JsonNode(JsonView view, int tok) : view_(view), tok_(tok) {}

  bool Exists() const;
  bool IsNull() const;
  bool Is(int type) const;
  bool IsObject() const;
  bool IsArray() const;
  bool IsString() const;
  bool IsPrimitive() const;

  explicit operator bool() const;

  int Tok() const;
  int Type() const;
  std::string_view Raw() const;
  size_t Count() const;

  std::optional<std::string_view> Str() const;
  std::optional<int32_t> Int() const;
  std::optional<float> Float() const;
  std::optional<bool> Bool() const;
  bool Ip(uint8_t out[4]) const;

  JsonNode operator[](std::string_view key) const;
  JsonNode operator[](size_t idx) const;

  bool operator==(std::string_view val) const;
  bool operator!=(std::string_view val) const { return !(*this == val); }

private:
  JsonView view_;
  int tok_ = -1;
};

enum class JsonParseError : int8_t
{
  None = 0,
  NoMemory = -1,
  Invalid = -2,
  Partial = -3,
};

class JsonParser
{
public:
  JsonParser() = default;

  bool Parse(const char *json, size_t len, Token *tokens, int token_cap);

  int TokenCount() const { return count_; }
  bool ok() const { return error_ == JsonParseError::None; }
  JsonParseError error() const { return error_; }
  JsonView View() const { return JsonView(json_, tokens_, count_); }

private:
  const char *json_ = nullptr;
  Token *tokens_ = nullptr;
  int count_ = 0;
  JsonParseError error_ = JsonParseError::None;
};

bool JsonParseInt(std::string_view s, int32_t &out);
bool JsonParseFloat(std::string_view s, float &out);
bool JsonParseBool(std::string_view s, bool &out);
bool JsonParseIp(std::string_view s, uint8_t out[4]);

void JsonCopyStr(char *dst, size_t cap, std::string_view sv);

template <typename Fn>
void JsonForEachPair(const JsonView &v, int obj, Fn fn)
{
  int count = v.Tok(obj).size;
  int it = obj + 1;
  for (int k = 0; k < count; k++)
  {
    int val = it + 1;
    if (!fn(v.View(it), val))
    {
      return;
    }
    it = v.Skip(val);
    if (it < 0)
    {
      return;
    }
  }
}

template <typename Fn>
void JsonForEachElement(const JsonView &v, int arr, Fn fn)
{
  int end = v.Tok(arr).end;
  int it = arr + 1;
  while ((it > 0) && (v.Tok(it).start < end))
  {
    if (!fn(it))
    {
      return;
    }
    it = v.Skip(it);
  }
}

}

#endif
