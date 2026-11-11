// SPDX-License-Identifier: MIT
#include "jsonlib/json_reader.hpp"

#include <cstring>

namespace jsonlib {

int JsonTokenSkip(const Token *toks, int count, int tok)
{
  if ((toks == nullptr) || (tok < 0) || (tok >= count))
  {
    return -1;
  }
  const int end = toks[tok].end;
  int i = tok + 1;
  while ((i < count) && (toks[i].start < end))
  {
    i++;
  }
  return (i < count) ? i : -1;
}

int JsonTokenFind(const char *buf, const Token *toks, int count, int obj,
                  std::string_view key)
{
  if ((buf == nullptr) || (toks == nullptr) || (obj < 0) || (obj >= count) ||
      (toks[obj].type != JSMN_OBJECT))
  {
    return -1;
  }
  const int members = toks[obj].size;
  int i = obj + 1;
  for (int k = 0; k < members; k++)
  {
    if ((i >= count) || (i + 1 >= count))
    {
      return -1;
    }
    if (toks[i].type == JSMN_STRING)
    {
      const Token &t = toks[i];
      const size_t len = (size_t)(t.end - t.start);
      if ((t.start >= 0) && (t.end >= t.start) && (len == key.size()) &&
          ((len == 0u) || (memcmp(buf + t.start, key.data(), len) == 0)))
      {
        return i + 1;
      }
    }
    const int value_end = toks[i + 1].end;
    int next = i + 2;
    while ((next < count) && (toks[next].start < value_end))
    {
      next++;
    }
    if (next >= count)
    {
      return -1;
    }
    i = next;
  }
  return -1;
}

std::string_view JsonTokenView(const char *buf, const Token *toks, int count,
                               int tok)
{
  if ((buf == nullptr) || (toks == nullptr) || (tok < 0) || (tok >= count))
  {
    return {};
  }
  const Token &t = toks[tok];
  if ((t.start < 0) || (t.end < t.start))
  {
    return {};
  }
  return std::string_view(buf + t.start, (size_t)(t.end - t.start));
}

}
