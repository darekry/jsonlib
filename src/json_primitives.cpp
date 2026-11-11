// SPDX-License-Identifier: MIT
#include "jsonlib/json_reader.hpp"
#include "jsonlib/config.hpp"

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace jsonlib {

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
