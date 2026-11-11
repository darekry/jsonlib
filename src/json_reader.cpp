// SPDX-License-Identifier: MIT
#include "jsonlib/json_reader.hpp"
#include "jsonlib/config.hpp"

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace jsonlib {

JsonView::JsonView(const char *buf, Token *toks, int n)
  : buf_(buf), toks_(toks), n_(n)
{
  JSONLIB_ASSERT(buf != nullptr);
  JSONLIB_ASSERT(toks != nullptr);
}

int JsonView::Find(int obj, std::string_view key) const
{
  if ((obj < 0) || (obj >= n_) || (toks_[obj].type != JSMN_OBJECT))
  {
    return -1;
  }
  int count = toks_[obj].size;
  int i = obj + 1;
  for (int k = 0; k < count; k++)
  {
    if (i + 1 >= n_)
    {
      return -1;
    }
    if ((toks_[i].type == JSMN_STRING) && (View(i) == key))
    {
      return i + 1;
    }
    i = Skip(i + 1);
    if (i < 0)
    {
      return -1;
    }
  }
  return -1;
}

std::string_view JsonView::View(int tok) const
{
  const Token &t = toks_[tok];
  return std::string_view(buf_ + t.start, (size_t)(t.end - t.start));
}

int JsonView::Type(int tok) const { return toks_[tok].type; }

const Token &JsonView::Tok(int i) const { return toks_[i]; }

bool JsonView::valid() const { return (buf_ != nullptr) && (toks_ != nullptr); }

int JsonView::Skip(int tok) const
{
  if ((tok < 0) || (tok >= n_))
  {
    return -1;
  }
  int end = toks_[tok].end;
  int i = tok + 1;
  while ((i < n_) && (toks_[i].start < end))
  {
    i++;
  }
  return (i < n_) ? i : -1;
}

bool JsonView::Contains(int tok) const
{
  return (tok >= 0) && (tok < n_);
}

JsonNode JsonView::At(int tok) const
{
  return JsonNode(*this, tok);
}

JsonNode JsonView::operator[](std::string_view key) const
{
  return JsonNode(*this, Find(0, key));
}

bool JsonNode::Exists() const
{
  return view_.Contains(tok_);
}

bool JsonNode::IsNull() const
{
  return IsPrimitive() && (view_.View(tok_) == "null");
}

bool JsonNode::Is(int type) const
{
  return Exists() && (view_.Type(tok_) == type);
}

bool JsonNode::IsObject() const { return Is(JSMN_OBJECT); }

bool JsonNode::IsArray() const { return Is(JSMN_ARRAY); }

bool JsonNode::IsString() const { return Is(JSMN_STRING); }

bool JsonNode::IsPrimitive() const { return Is(JSMN_PRIMITIVE); }

JsonNode::operator bool() const
{
  return Exists() && !IsNull();
}

int JsonNode::Tok() const
{
  return tok_;
}

int JsonNode::Type() const
{
  return Exists() ? view_.Type(tok_) : -1;
}

std::string_view JsonNode::Raw() const
{
  return Exists() ? view_.View(tok_) : std::string_view{};
}

size_t JsonNode::Count() const
{
  if (!IsObject() && !IsArray())
  {
    return 0u;
  }
  int count = view_.Tok(tok_).size;
  return (count > 0) ? (size_t)count : 0u;
}

std::optional<std::string_view> JsonNode::Str() const
{
  if (!IsString())
  {
    return std::nullopt;
  }
  return view_.View(tok_);
}

std::optional<int32_t> JsonNode::Int() const
{
  if (!IsPrimitive())
  {
    return std::nullopt;
  }
  int32_t val = 0;
  if (!JsonParseInt(view_.View(tok_), val))
  {
    return std::nullopt;
  }
  return val;
}

std::optional<float> JsonNode::Float() const
{
  if (!IsPrimitive())
  {
    return std::nullopt;
  }
  float val = 0.0f;
  if (!JsonParseFloat(view_.View(tok_), val))
  {
    return std::nullopt;
  }
  return val;
}

std::optional<bool> JsonNode::Bool() const
{
  if (!IsPrimitive())
  {
    return std::nullopt;
  }
  bool val = false;
  if (!JsonParseBool(view_.View(tok_), val))
  {
    return std::nullopt;
  }
  return val;
}

bool JsonNode::Ip(uint8_t out[4]) const
{
  return IsString() && JsonParseIp(view_.View(tok_), out);
}

JsonNode JsonNode::operator[](std::string_view key) const
{
  if (!IsObject())
  {
    return JsonNode{};
  }
  return JsonNode(view_, view_.Find(tok_, key));
}

JsonNode JsonNode::operator[](size_t idx) const
{
  if (!IsArray())
  {
    return JsonNode{};
  }
  int count = view_.Tok(tok_).size;
  if ((count <= 0) || (idx >= (size_t)count))
  {
    return JsonNode{};
  }
  int tok = tok_ + 1;
  for (size_t i = 0u; i < idx; i++)
  {
    tok = view_.Skip(tok);
    if (tok < 0)
    {
      return JsonNode{};
    }
  }
  return JsonNode(view_, tok);
}

bool JsonNode::operator==(std::string_view val) const
{
  return Exists() && (view_.View(tok_) == val);
}

bool JsonParser::Parse(const char *json, size_t len, Token *tokens, int token_cap)
{
  json_ = json;
  tokens_ = tokens;
  count_ = 0;
  error_ = JsonParseError::None;

  if ((json == nullptr) || (tokens == nullptr) || (token_cap <= 0))
  {
    error_ = JsonParseError::NoMemory;
    return false;
  }

  jsmn_parser parser;
  jsmn_init(&parser);
  int n = jsmn_parse(&parser, json, len, tokens, (unsigned)token_cap);
  if (n < 0)
  {
    switch (n)
    {
      case JSMN_ERROR_NOMEM: error_ = JsonParseError::NoMemory; break;
      case JSMN_ERROR_INVAL: error_ = JsonParseError::Invalid; break;
      default:               error_ = JsonParseError::Partial; break;
    }
    return false;
  }

  count_ = n;
  return true;
}

bool JsonParseInt(std::string_view s, int32_t &out)
{
  int32_t v = 0;
  const char *last = s.data() + s.size();
  auto [ptr, errc] = std::from_chars(s.data(), last, v, 10);
  if ((errc != std::errc{}) || (ptr != last))
  {
    return false;
  }
  out = v;
  return true;
}

bool JsonParseFloat(std::string_view s, float &out)
{
  char buf[64];
  if (s.empty() || (s.size() >= sizeof(buf)))
  {
    return false;
  }
  memcpy(buf, s.data(), s.size());
  buf[s.size()] = '\0';
  errno = 0;
  char *end = nullptr;
  float v = std::strtof(buf, &end);
  if ((end != (buf + s.size())) || (errno == ERANGE) || !std::isfinite(v))
  {
    return false;
  }
  out = v;
  return true;
}

bool JsonParseBool(std::string_view s, bool &out)
{
  if (s == "true")
  {
    out = true;
    return true;
  }
  if (s == "false")
  {
    out = false;
    return true;
  }
  return false;
}

bool JsonParseIp(std::string_view s, uint8_t out[4])
{
  size_t i = 0u;
  uint8_t ip[4] = { 0u, 0u, 0u, 0u };
  for (int part = 0; part < 4; part++)
  {
    int val = 0;
    auto [p, ec] = std::from_chars(s.data() + i, s.data() + s.size(), val);
    if ((ec != std::errc{}) || ((p - (s.data() + i)) > 3) || (val < 0) ||
        (val > 255))
    {
      return false;
    }
    ip[part] = (uint8_t)val;
    i = (size_t)(p - s.data());
    if (part < 3)
    {
      if ((i >= s.size()) || (s[i] != '.'))
      {
        return false;
      }
      i++;
    }
  }
  if (i != s.size())
  {
    return false;
  }
  memcpy(out, ip, sizeof(ip));
  return true;
}

void JsonCopyStr(char *dst, size_t cap, std::string_view sv)
{
  JSONLIB_ASSERT(dst != nullptr);
  if (cap == 0u)
  {
    return;
  }
  size_t n = sv.size();
  if (n > (cap - 1u))
  {
    n = cap - 1u;
  }
  memcpy(dst, sv.data(), n);
  dst[n] = '\0';
}

}
