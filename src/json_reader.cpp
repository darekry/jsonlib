// SPDX-License-Identifier: MIT
#include "jsonlib/json_reader.hpp"
#include "jsonlib/config.hpp"

namespace jsonlib {

int JsonParseTokens(const char *json, size_t len, Token *tokens, int token_cap)
{
  if ((json == nullptr) || (tokens == nullptr) || (token_cap <= 0) ||
      (len == 0u))
  {
    return JSMN_ERROR_INVAL;
  }

  jsmn_parser parser;
  jsmn_init(&parser);
  return jsmn_parse(&parser, json, len, tokens, (unsigned)token_cap);
}

bool JsonTokenContains(int count, int tok)
{
  return (tok >= 0) && (tok < count);
}

int JsonTokenSkip(const Token *toks, int count, int tok)
{
  if ((toks == nullptr) || !JsonTokenContains(count, tok))
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
  if ((buf == nullptr) || (toks == nullptr) ||
      !JsonTokenContains(count, obj) ||
      (toks[obj].type != JSMN_OBJECT))
  {
    return -1;
  }
  const int members = toks[obj].size;
  int i = obj + 1;
  for (int k = 0; k < members; k++)
  {
    if (!JsonTokenContains(count, i) || !JsonTokenContains(count, i + 1))
    {
      return -1;
    }
    if ((toks[i].type == JSMN_STRING) &&
        (JsonTokenView(buf, toks, count, i) == key))
    {
      return i + 1;
    }
    i = JsonTokenSkip(toks, count, i + 1);
    if (i < 0)
    {
      return -1;
    }
  }
  return -1;
}

std::string_view JsonTokenView(const char *buf, const Token *toks, int count,
                               int tok)
{
  if ((buf == nullptr) || (toks == nullptr) ||
      !JsonTokenContains(count, tok))
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
