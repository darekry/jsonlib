// SPDX-License-Identifier: MIT
#ifndef JSONLIB_JSON_SPAN_READER_HPP
#define JSONLIB_JSON_SPAN_READER_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

#include "jsonlib/config.hpp"

namespace jsonlib {

enum class JsonReadError : uint8_t
{
  None = 0,
  Syntax,
  Partial,
  NoMemory,
  TooDeep,
};

enum class JsonSpanType : uint8_t
{
  None = 0,
  Object,
  Array,
  String,
  Primitive,
};

/* Non-owning slice of one JSON token. Containers cover the braces, strings
 * cover the quotes; content() strips the quotes for strings. */
struct JsonSpan
{
  const char *begin = nullptr;
  const char *end = nullptr;
  JsonSpanType type = JsonSpanType::None;

  bool valid() const
  {
    return (begin != nullptr) && (end != nullptr) && (end >= begin);
  }

  size_t size() const
  {
    return valid() ? (size_t)(end - begin) : 0u;
  }

  std::string_view full() const;
  std::string_view content() const;
};

namespace detail {

inline bool IsDigit(char c)
{
  return (c >= '0') && (c <= '9');
}

inline bool IsHexDigit(char c)
{
  return IsDigit(c) || ((c >= 'a') && (c <= 'f')) ||
         ((c >= 'A') && (c <= 'F'));
}

inline bool IsWhitespace(char c)
{
  return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}

inline bool IsValueDelimiter(char c)
{
  return IsWhitespace(c) || (c == ',') || (c == ']') || (c == '}');
}

/* Strict JSON number grammar:
 * -?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)? */
inline bool ValidateNumber(std::string_view text)
{
  size_t i = 0u;
  const size_t n = text.size();
  if ((i < n) && (text[i] == '-'))
  {
    i++;
  }
  if (i >= n)
  {
    return false;
  }
  if (text[i] == '0')
  {
    i++;
  }
  else if ((text[i] >= '1') && (text[i] <= '9'))
  {
    do
    {
      i++;
    } while ((i < n) && IsDigit(text[i]));
  }
  else
  {
    return false;
  }
  if ((i < n) && (text[i] == '.'))
  {
    i++;
    if ((i >= n) || !IsDigit(text[i]))
    {
      return false;
    }
    while ((i < n) && IsDigit(text[i]))
    {
      i++;
    }
  }
  if ((i < n) && ((text[i] == 'e') || (text[i] == 'E')))
  {
    i++;
    if ((i < n) && ((text[i] == '+') || (text[i] == '-')))
    {
      i++;
    }
    if ((i >= n) || !IsDigit(text[i]))
    {
      return false;
    }
    while ((i < n) && IsDigit(text[i]))
    {
      i++;
    }
  }
  return i == n;
}

inline bool ValidatePrimitive(std::string_view text)
{
  const char *p = text.data();
  const size_t n = text.size();
  if (((n == 4u) && ((memcmp(p, "true", 4u) == 0) ||
                     (memcmp(p, "null", 4u) == 0))) ||
      ((n == 5u) && (memcmp(p, "false", 5u) == 0)))
  {
    return true;
  }
  return ValidateNumber(text);
}

inline bool ValidateStringContent(const char *begin, const char *end)
{
  for (const char *p = begin; p < end; ++p)
  {
    if ((unsigned char)*p < 0x20u)
    {
      return false;
    }
  }
  return true;
}

const char *SkipWhitespace(const char *pos, const char *end);
const char *ScanString(const char *pos, const char *end,
                       const char *&content_begin, const char *&content_end,
                       JsonReadError &err);
const char *ScanValue(const char *pos, const char *end, int depth,
                      JsonSpan &out, JsonReadError &err);
bool ParseDocument(const char *json, size_t len, JsonSpan &root,
                   JsonReadError &err);

bool FindMember(const JsonSpan &object, std::string_view key, JsonSpan &out);
bool FindElement(const JsonSpan &array, size_t index, JsonSpan &out);
size_t CountEntries(const JsonSpan &container);

bool NextElement(const JsonSpan &array, const char *&pos, JsonSpan &out);
bool NextMember(const JsonSpan &object, const char *&pos, std::string_view &key,
                JsonSpan &out);

}
}

#endif
