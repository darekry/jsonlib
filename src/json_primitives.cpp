// SPDX-License-Identifier: MIT
#include "jsonlib/json_reader.hpp"
#include "jsonlib/config.hpp"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace jsonlib {

bool JsonParseInt(std::string_view s, int32_t &out)
{
  if (s.empty())
  {
    return false;
  }
  const bool neg = (s[0] == '-');
  const uint32_t limit = neg ? 2147483648u : 2147483647u;
  const uint32_t q = limit / 10u;
  const uint32_t r = limit % 10u;
  size_t i = neg ? 1u : 0u;
  uint32_t v = 0u;
  for (; i < s.size(); i++)
  {
    const char c = s[i];
    if ((c < '0') || (c > '9'))
    {
      return false;
    }
    const uint32_t d = (uint32_t)(c - '0');
    if ((v > q) || ((v == q) && (d > r)))
    {
      return false;
    }
    v = (v * 10u) + d;
  }
  if (i == (neg ? 1u : 0u))
  {
    return false;
  }
  if (neg)
  {
    out = (v == 2147483648u) ? (-2147483647 - 1) : -(int32_t)v;
  }
  else
  {
    out = (int32_t)v;
  }
  return true;
}

#if !JSONLIB_NO_FLOAT
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
#endif

bool JsonParseBool(std::string_view s, bool &out)
{
  if ((s.size() == 4u) && (memcmp(s.data(), "true", 4u) == 0))
  {
    out = true;
    return true;
  }
  if ((s.size() == 5u) && (memcmp(s.data(), "false", 5u) == 0))
  {
    out = false;
    return true;
  }
  return false;
}

bool JsonParseIp(std::string_view s, uint8_t out[4])
{
  uint8_t ip[4];
  size_t i = 0u;
  for (int part = 0; part < 4; part++)
  {
    const size_t begin = i;
    bool neg = false;
    if ((i < s.size()) && (s[i] == '-'))
    {
      neg = true;
      i++;
    }
    const size_t digits = i;
    int val = 0;
    while ((i < s.size()) && (s[i] >= '0') && (s[i] <= '9') &&
           ((i - begin) < 3u))
    {
      val = (val * 10) + (s[i] - '0');
      i++;
    }
    if ((i == digits) ||
        ((i < s.size()) && (s[i] >= '0') && (s[i] <= '9')) ||
        (val > 255) || (neg && (val != 0)))
    {
      return false;
    }
    ip[part] = (uint8_t)val;
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
