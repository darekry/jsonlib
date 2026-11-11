// SPDX-License-Identifier: MIT
#include "jsonlib/json_span_reader.hpp"

namespace jsonlib {

std::string_view JsonSpan::full() const
{
  if (!valid())
  {
    return {};
  }
  return std::string_view(begin, (size_t)(end - begin));
}

std::string_view JsonSpan::content() const
{
  if (!valid())
  {
    return {};
  }
  if (type != JsonSpanType::String)
  {
    return full();
  }
  if ((size_t)(end - begin) < 2u)
  {
    return {};
  }
  return std::string_view(begin + 1, (size_t)(end - begin) - 2u);
}

namespace detail {

const char *SkipWhitespace(const char *pos, const char *end)
{
  while ((pos < end) && IsWhitespace(*pos))
  {
    pos++;
  }
  return pos;
}

const char *ScanString(const char *pos, const char *end,
                       const char *&content_begin, const char *&content_end,
                       JsonReadError &err)
{
  if ((pos == nullptr) || (pos >= end) || (*pos != '"'))
  {
    err = JsonReadError::Syntax;
    return nullptr;
  }
  const char *p = pos + 1;
  content_begin = p;
  while (p < end)
  {
    const char c = *p;
    if (c == '"')
    {
      content_end = p;
      err = JsonReadError::None;
      return p + 1;
    }
    if (c == '\\')
    {
      p++;
      if (p >= end)
      {
        err = JsonReadError::Partial;
        return nullptr;
      }
      const char esc = *p;
      if ((esc == '"') || (esc == '/') || (esc == '\\') || (esc == 'b') ||
          (esc == 'f') || (esc == 'r') || (esc == 'n') || (esc == 't'))
      {
        p++;
        continue;
      }
      if (esc != 'u')
      {
        err = JsonReadError::Syntax;
        return nullptr;
      }
      p++;
      for (int i = 0; i < 4; i++)
      {
        if (p >= end)
        {
          err = JsonReadError::Partial;
          return nullptr;
        }
        if (!IsHexDigit(*p))
        {
          err = JsonReadError::Syntax;
          return nullptr;
        }
        p++;
      }
      continue;
    }
    if ((unsigned char)c < 0x20u)
    {
      err = JsonReadError::Syntax;
      return nullptr;
    }
    p++;
  }
  err = JsonReadError::Partial;
  return nullptr;
}

const char *ScanValue(const char *pos, const char *end, int depth,
                      JsonSpan &out, JsonReadError &err)
{
  const char *p = SkipWhitespace(pos, end);
  if (p >= end)
  {
    err = JsonReadError::Partial;
    return nullptr;
  }
  const char c = *p;
  if (c == '"')
  {
    const char *content_begin = nullptr;
    const char *content_end = nullptr;
    const char *next =
      ScanString(p, end, content_begin, content_end, err);
    if (next == nullptr)
    {
      return nullptr;
    }
    out = JsonSpan{ p, next, JsonSpanType::String };
    err = JsonReadError::None;
    return next;
  }
  if ((c == '{') || (c == '['))
  {
    if ((depth + 1) > JSONLIB_JSON_MAX_DEPTH)
    {
      err = JsonReadError::TooDeep;
      return nullptr;
    }
    const char *start = p;
    const bool is_object = (c == '{');
    const char close = is_object ? '}' : ']';
    p++;
    p = SkipWhitespace(p, end);
    if (p >= end)
    {
      err = JsonReadError::Partial;
      return nullptr;
    }
    if (*p == close)
    {
      p++;
      out = JsonSpan{ start, p,
                      is_object ? JsonSpanType::Object : JsonSpanType::Array };
      err = JsonReadError::None;
      return p;
    }
    for (;;)
    {
      if (is_object)
      {
        const char *key_begin = nullptr;
        const char *key_end = nullptr;
        const char *key_next =
          ScanString(p, end, key_begin, key_end, err);
        if (key_next == nullptr)
        {
          return nullptr;
        }
        p = SkipWhitespace(key_next, end);
        if (p >= end)
        {
          err = JsonReadError::Partial;
          return nullptr;
        }
        if (*p != ':')
        {
          err = JsonReadError::Syntax;
          return nullptr;
        }
        p++;
      }
      JsonSpan value;
      p = ScanValue(p, end, depth + 1, value, err);
      if (p == nullptr)
      {
        return nullptr;
      }
      p = SkipWhitespace(p, end);
      if (p >= end)
      {
        err = JsonReadError::Partial;
        return nullptr;
      }
      if (*p == ',')
      {
        p++;
        p = SkipWhitespace(p, end);
        if (p >= end)
        {
          err = JsonReadError::Partial;
          return nullptr;
        }
        if (*p == close)
        {
          err = JsonReadError::Syntax;
          return nullptr;
        }
        continue;
      }
      if (*p == close)
      {
        p++;
        out = JsonSpan{ start, p,
                        is_object ? JsonSpanType::Object
                                  : JsonSpanType::Array };
        err = JsonReadError::None;
        return p;
      }
      err = JsonReadError::Syntax;
      return nullptr;
    }
  }
  if ((c == '-') || IsDigit(c) || (c == 't') || (c == 'f') || (c == 'n'))
  {
    const char *start = p;
    while ((p < end) && !IsValueDelimiter(*p))
    {
      p++;
    }
    if (p >= end)
    {
      err = JsonReadError::Partial;
      return nullptr;
    }
    const std::string_view raw(start, (size_t)(p - start));
    if (!ValidatePrimitive(raw))
    {
      err = JsonReadError::Syntax;
      return nullptr;
    }
    out = JsonSpan{ start, p, JsonSpanType::Primitive };
    err = JsonReadError::None;
    return p;
  }
  err = JsonReadError::Syntax;
  return nullptr;
}

bool ParseDocument(const char *json, size_t len, JsonSpan &root,
                   JsonReadError &err)
{
  if (json == nullptr)
  {
    err = JsonReadError::Syntax;
    return false;
  }
  const char *end = json + len;
  const char *p = SkipWhitespace(json, end);
  if (p >= end)
  {
    err = JsonReadError::Syntax;
    return false;
  }
  JsonSpan value;
  p = ScanValue(p, end, 0, value, err);
  if (p == nullptr)
  {
    return false;
  }
  p = SkipWhitespace(p, end);
  if (p != end)
  {
    err = JsonReadError::Syntax;
    return false;
  }
  root = value;
  err = JsonReadError::None;
  return true;
}

bool FindMember(const JsonSpan &object, std::string_view key, JsonSpan &out)
{
  if (!object.valid() || (object.type != JsonSpanType::Object))
  {
    return false;
  }
  const char *end = object.end - 1;
  const char *limit = object.end;
  const char *pos = object.begin + 1;
  JsonSpan value;
  for (;;)
  {
    const char *p = SkipWhitespace(pos, end);
    if (p >= end)
    {
      return false;
    }
    if (*p == ',')
    {
      ++p;
      p = SkipWhitespace(p, end);
      if (p >= end)
      {
        return false;
      }
    }
    const char *key_begin = nullptr;
    const char *key_end = nullptr;
    JsonReadError err = JsonReadError::None;
    const char *next = ScanString(p, end, key_begin, key_end, err);
    if (next == nullptr)
    {
      return false;
    }
    p = SkipWhitespace(next, end);
    if ((p >= end) || (*p != ':'))
    {
      return false;
    }
    p++;
    next = ScanValue(p, limit, 0, value, err);
    if (next == nullptr)
    {
      return false;
    }
    if (std::string_view(key_begin, (size_t)(key_end - key_begin)) == key)
    {
      out = value;
      return true;
    }
    pos = SkipWhitespace(next, end);
  }
}

bool FindElement(const JsonSpan &array, size_t index, JsonSpan &out)
{
  if (!array.valid() || (array.type != JsonSpanType::Array))
  {
    return false;
  }
  const char *end = array.end - 1;
  const char *limit = array.end;
  const char *pos = array.begin + 1;
  size_t current = 0u;
  JsonSpan value;
  for (;;)
  {
    const char *p = SkipWhitespace(pos, end);
    if (p >= end)
    {
      return false;
    }
    if (*p == ',')
    {
      ++p;
      p = SkipWhitespace(p, end);
      if (p >= end)
      {
        return false;
      }
    }
    JsonReadError err = JsonReadError::None;
    const char *next = ScanValue(p, limit, 0, value, err);
    if (next == nullptr)
    {
      return false;
    }
    if (current == index)
    {
      out = value;
      return true;
    }
    current++;
    pos = SkipWhitespace(next, end);
  }
}

bool NextElement(const JsonSpan &array, const char *&pos, JsonSpan &out)
{
  if (!array.valid() || (array.type != JsonSpanType::Array))
  {
    return false;
  }
  const char *end = array.end - 1;
  const char *limit = array.end;
  const char *p = SkipWhitespace(pos, end);
  if (p >= end)
  {
    pos = p;
    return false;
  }
  if (*p == ',')
  {
    ++p;
    p = SkipWhitespace(p, end);
    if (p >= end)
    {
      pos = p;
      return false;
    }
  }
  JsonReadError err = JsonReadError::None;
  const char *next = ScanValue(p, limit, 0, out, err);
  if (next == nullptr)
  {
    pos = end;
    return false;
  }
  pos = next;
  return true;
}

bool NextMember(const JsonSpan &object, const char *&pos, std::string_view &key,
                JsonSpan &out)
{
  if (!object.valid() || (object.type != JsonSpanType::Object))
  {
    return false;
  }
  const char *end = object.end - 1;
  const char *limit = object.end;
  const char *p = SkipWhitespace(pos, end);
  if (p >= end)
  {
    pos = p;
    return false;
  }
  if (*p == ',')
  {
    ++p;
    p = SkipWhitespace(p, end);
    if (p >= end)
    {
      pos = p;
      return false;
    }
  }
  const char *key_begin = nullptr;
  const char *key_end = nullptr;
  JsonReadError err = JsonReadError::None;
  const char *next = ScanString(p, end, key_begin, key_end, err);
  if (next == nullptr)
  {
    pos = end;
    return false;
  }
  p = SkipWhitespace(next, end);
  if ((p >= end) || (*p != ':'))
  {
    pos = end;
    return false;
  }
  p++;
  next = ScanValue(p, limit, 0, out, err);
  if (next == nullptr)
  {
    pos = end;
    return false;
  }
  key = std::string_view(key_begin, (size_t)(key_end - key_begin));
  pos = next;
  return true;
}

size_t CountEntries(const JsonSpan &container)
{
  if (!container.valid())
  {
    return 0u;
  }
  size_t count = 0u;
  if (container.type == JsonSpanType::Array)
  {
    const char *pos = container.begin + 1;
    JsonSpan value;
    while (NextElement(container, pos, value))
    {
      count++;
    }
  }
  else if (container.type == JsonSpanType::Object)
  {
    const char *pos = container.begin + 1;
    JsonSpan value;
    std::string_view key;
    while (NextMember(container, pos, key, value))
    {
      count++;
    }
  }
  return count;
}

}
}
