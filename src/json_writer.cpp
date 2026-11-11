// SPDX-License-Identifier: MIT
#include "jsonlib/json_writer.hpp"
#include "jsonlib/config.hpp"

#include <cstdio>
#include <cstring>

namespace jsonlib {

namespace {

char *FormatU32(char *end, uint32_t val)
{
  do
  {
    *--end = (char)('0' + (val % 10u));
    val /= 10u;
  } while (val != 0u);
  return end;
}

}

JsonBuilder::JsonBuilder(char *buf, size_t cap)
{
  buf_ = buf;
  cap_ = cap;
  len_ = 0u;
  ok_ = false;
  depth_ = 0u;
  pending_key_ = false;
  if ((buf_ == nullptr) || (cap_ == 0u))
  {
    buf_ = nullptr;
    return;
  }
  ok_ = true;
  buf_[0] = '\0';
}

void JsonBuilder::BeforeKey()
{
  if (!ok_)
  {
    return;
  }
  if ((depth_ == 0u) || ((st_[depth_ - 1u] & kTypeMask) != 'O') ||
      pending_key_)
  {
    ok_ = false;
    return;
  }
  if ((st_[depth_ - 1u] & kFirst) == 0u)
  {
    AppendChar(',');
    if (!ok_)
    {
      return;
    }
  }
}

void JsonBuilder::BeforeValue()
{
  if (!ok_)
  {
    return;
  }
  if (pending_key_)
  {
    return;
  }
  if (depth_ == 0u)
  {
    if (len_ > 0u)
    {
      ok_ = false;
    }
    return;
  }
  if ((st_[depth_ - 1u] & kTypeMask) != 'A')
  {
    ok_ = false;
    return;
  }
  if ((st_[depth_ - 1u] & kFirst) == 0u)
  {
    AppendChar(',');
  }
}

void JsonBuilder::AfterValue()
{
  if (!ok_)
  {
    return;
  }
  pending_key_ = false;
  if (depth_ > 0u)
  {
    st_[depth_ - 1u] &= (uint8_t)~kFirst;
  }
}

void JsonBuilder::AppendChar(char c)
{
  if (!ok_)
  {
    return;
  }
  if (len_ + 1u + 1u > cap_)
  {
    ok_ = false;
    return;
  }
  buf_[len_] = c;
  len_++;
  buf_[len_] = '\0';
}

void JsonBuilder::AppendRaw(const char *p, size_t n)
{
  if (!ok_ || (n == 0u))
  {
    return;
  }
  JSONLIB_ASSERT(p != nullptr);
  if (len_ + n + 1u > cap_)
  {
    ok_ = false;
    return;
  }
  memcpy(buf_ + len_, p, n);
  len_ += n;
  buf_[len_] = '\0';
}

void JsonBuilder::AppendQuoted(std::string_view s)
{
  static constexpr char kHex[] = "0123456789ABCDEF";
  const char *p = s.data();
  const size_t n = s.size();
  AppendChar('"');
  if (!ok_)
  {
    return;
  }
  size_t i = 0u;
  while (ok_ && (i < n))
  {
    const char c = p[i];
    if ((c != '"') && (c != '\\') && ((unsigned char)c >= 0x20u))
    {
      size_t j = i + 1u;
      while ((j < n) && (p[j] != '"') && (p[j] != '\\') &&
             ((unsigned char)p[j] >= 0x20u))
      {
        j++;
      }
      AppendRaw(p + i, j - i);
      i = j;
      continue;
    }
    switch (c)
    {
      case '"':  AppendRaw("\\\"", 2u); break;
      case '\\': AppendRaw("\\\\", 2u); break;
      case '\n': AppendRaw("\\n", 2u); break;
      case '\r': AppendRaw("\\r", 2u); break;
      case '\t': AppendRaw("\\t", 2u); break;
      case '\b': AppendRaw("\\b", 2u); break;
      case '\f': AppendRaw("\\f", 2u); break;
      default:
      {
        char esc[6] = { '\\', 'u', '0', '0', '0', '0' };
        esc[4] = kHex[(c >> 4) & 0x0F];
        esc[5] = kHex[c & 0x0F];
        AppendRaw(esc, sizeof(esc));
        break;
      }
    }
    i++;
  }
  AppendChar('"');
}

JsonBuilder &JsonBuilder::Open(uint8_t type, char bracket)
{
  const bool had_key = pending_key_;
  const uint8_t parent = depth_;
  BeforeValue();
  if (!ok_)
  {
    return *this;
  }
  if (depth_ >= kMaxDepth)
  {
    ok_ = false;
    return *this;
  }
  AppendChar(bracket);
  if (!ok_)
  {
    return *this;
  }
  st_[depth_] = (uint8_t)(type | kFirst);
  depth_++;
  if (had_key)
  {
    pending_key_ = false;
    if (parent > 0u)
    {
      st_[parent - 1u] &= (uint8_t)~kFirst;
    }
  }
  return *this;
}

JsonBuilder &JsonBuilder::Close(uint8_t type, char bracket)
{
  if (!ok_ || (depth_ == 0u) || ((st_[depth_ - 1u] & kTypeMask) != type) ||
      pending_key_)
  {
    ok_ = false;
    return *this;
  }
  depth_--;
  AppendChar(bracket);
  AfterValue();
  return *this;
}

JsonBuilder &JsonBuilder::BeginObject()
{
  return Open('O', '{');
}

JsonBuilder &JsonBuilder::EndObject()
{
  return Close('O', '}');
}

JsonBuilder &JsonBuilder::BeginArray()
{
  return Open('A', '[');
}

JsonBuilder &JsonBuilder::EndArray()
{
  return Close('A', ']');
}

JsonBuilder &JsonBuilder::Key(std::string_view key)
{
  BeforeKey();
  if (!ok_)
  {
    return *this;
  }
  AppendQuoted(key);
  if (!ok_)
  {
    return *this;
  }
  AppendChar(':');
  if (!ok_)
  {
    return *this;
  }
  pending_key_ = true;
  return *this;
}

JsonBuilder &JsonBuilder::Key(const char *key)
{
  JSONLIB_ASSERT(key != nullptr);
  return Key(std::string_view(key, strlen(key)));
}

JsonBuilder &JsonBuilder::Val(std::string_view val)
{
  BeforeValue();
  if (!ok_)
  {
    return *this;
  }
  AppendQuoted(val);
  AfterValue();
  return *this;
}

JsonBuilder &JsonBuilder::Val(const char *val)
{
  JSONLIB_ASSERT(val != nullptr);
  return Val(std::string_view(val, strlen(val)));
}

JsonBuilder &JsonBuilder::Val(int32_t val)
{
  BeforeValue();
  if (!ok_)
  {
    return *this;
  }
  char tmp[11];
  char *end = tmp + sizeof(tmp);
  const uint32_t mag = (val < 0) ? (0u - (uint32_t)val) : (uint32_t)val;
  char *begin = FormatU32(end, mag);
  if (val < 0)
  {
    *--begin = '-';
  }
  AppendRaw(begin, (size_t)(end - begin));
  AfterValue();
  return *this;
}

JsonBuilder &JsonBuilder::Val(uint32_t val)
{
  BeforeValue();
  if (!ok_)
  {
    return *this;
  }
  char tmp[10];
  char *end = tmp + sizeof(tmp);
  char *begin = FormatU32(end, val);
  AppendRaw(begin, (size_t)(end - begin));
  AfterValue();
  return *this;
}

JsonBuilder &JsonBuilder::Val(bool val)
{
  BeforeValue();
  if (!ok_)
  {
    return *this;
  }
  AppendRaw(val ? "true" : "false", val ? 4u : 5u);
  AfterValue();
  return *this;
}

#if !JSONLIB_NO_FLOAT
JsonBuilder &JsonBuilder::Val(float val)
{
  BeforeValue();
  if (!ok_)
  {
    return *this;
  }
  /* %.9g: pełna precyzja float, deterministyczny na nano-libc. */
  char tmp[32];
  int n = snprintf(tmp, sizeof(tmp), "%.9g", (double)val);
  if ((n <= 0) || ((size_t)n >= sizeof(tmp)))
  {
    ok_ = false;
    return *this;
  }
  AppendRaw(tmp, (size_t)n);
  AfterValue();
  return *this;
}

JsonBuilder &JsonBuilder::Val(double val)
{
  BeforeValue();
  if (!ok_)
  {
    return *this;
  }
  char tmp[32];
  int n = snprintf(tmp, sizeof(tmp), "%.17g", val);
  if ((n <= 0) || ((size_t)n >= sizeof(tmp)))
  {
    ok_ = false;
    return *this;
  }
  AppendRaw(tmp, (size_t)n);
  AfterValue();
  return *this;
}
#endif

JsonBuilder &JsonBuilder::Null()
{
  BeforeValue();
  if (!ok_)
  {
    return *this;
  }
  AppendRaw("null", 4u);
  AfterValue();
  return *this;
}

JsonBuilder::ObjectScope JsonBuilder::Object()
{
  return ObjectScope(*this);
}

JsonBuilder::ArrayScope JsonBuilder::Array()
{
  return ArrayScope(*this);
}

JsonBuilder &JsonBuilder::Raw(std::string_view frag)
{
  if (!ok_ || frag.empty())
  {
    return *this;
  }
  AppendRaw(frag.data(), frag.size());
  return *this;
}

JsonBuilder &JsonBuilder::Pair(std::string_view key, std::string_view val)
{
  Key(key);
  Val(val);
  return *this;
}

JsonBuilder &JsonBuilder::Pair(std::string_view key, const char *val)
{
  JSONLIB_ASSERT(val != nullptr);
  return Pair(key, std::string_view(val, strlen(val)));
}

JsonBuilder &JsonBuilder::Pair(const char *key, std::string_view val)
{
  JSONLIB_ASSERT(key != nullptr);
  return Pair(std::string_view(key, strlen(key)), val);
}

JsonBuilder &JsonBuilder::Pair(const char *key, const char *val)
{
  JSONLIB_ASSERT((key != nullptr) && (val != nullptr));
  return Pair(std::string_view(key, strlen(key)),
              std::string_view(val, strlen(val)));
}

JsonBuilder &JsonBuilder::Pair(std::string_view key, int32_t val)
{
  Key(key);
  Val(val);
  return *this;
}

JsonBuilder &JsonBuilder::Pair(const char *key, int32_t val)
{
  JSONLIB_ASSERT(key != nullptr);
  return Pair(std::string_view(key, strlen(key)), val);
}

JsonBuilder &JsonBuilder::Pair(std::string_view key, uint32_t val)
{
  Key(key);
  Val(val);
  return *this;
}

JsonBuilder &JsonBuilder::Pair(const char *key, uint32_t val)
{
  JSONLIB_ASSERT(key != nullptr);
  return Pair(std::string_view(key, strlen(key)), val);
}

JsonBuilder &JsonBuilder::Pair(std::string_view key, bool val)
{
  Key(key);
  Val(val);
  return *this;
}

JsonBuilder &JsonBuilder::Pair(const char *key, bool val)
{
  JSONLIB_ASSERT(key != nullptr);
  return Pair(std::string_view(key, strlen(key)), val);
}

#if !JSONLIB_NO_FLOAT
JsonBuilder &JsonBuilder::Pair(std::string_view key, float val)
{
  Key(key);
  Val(val);
  return *this;
}

JsonBuilder &JsonBuilder::Pair(const char *key, float val)
{
  JSONLIB_ASSERT(key != nullptr);
  return Pair(std::string_view(key, strlen(key)), val);
}

JsonBuilder &JsonBuilder::Pair(std::string_view key, double val)
{
  Key(key);
  Val(val);
  return *this;
}

JsonBuilder &JsonBuilder::Pair(const char *key, double val)
{
  JSONLIB_ASSERT(key != nullptr);
  return Pair(std::string_view(key, strlen(key)), val);
}
#endif

void JsonBuilder::Fail()
{
  ok_ = false;
}

bool JsonBuilder::ok() const { return ok_; }

size_t JsonBuilder::len() const { return len_; }

const char *JsonBuilder::c_str() const { return (buf_ != nullptr) ? buf_ : ""; }

JsonBuilder::KeyRef JsonBuilder::operator[](std::string_view key)
{
  return KeyRef(*this, key);
}

JsonBuilder::KeyRef JsonBuilder::operator[](const char *key)
{
  JSONLIB_ASSERT(key != nullptr);
  return KeyRef(*this, std::string_view(key, strlen(key)));
}

JsonBuilder::KeyRef &JsonBuilder::KeyRef::operator=(std::string_view val)
{
  builder_->Pair(key_, val);
  return *this;
}

JsonBuilder::KeyRef &JsonBuilder::KeyRef::operator=(const char *val)
{
  builder_->Pair(key_, val);
  return *this;
}

JsonBuilder::KeyRef &JsonBuilder::KeyRef::operator=(int32_t val)
{
  builder_->Pair(key_, val);
  return *this;
}

JsonBuilder::KeyRef &JsonBuilder::KeyRef::operator=(uint32_t val)
{
  builder_->Pair(key_, val);
  return *this;
}

JsonBuilder::KeyRef &JsonBuilder::KeyRef::operator=(bool val)
{
  builder_->Pair(key_, val);
  return *this;
}

#if !JSONLIB_NO_FLOAT
JsonBuilder::KeyRef &JsonBuilder::KeyRef::operator=(float val)
{
  builder_->Pair(key_, val);
  return *this;
}

JsonBuilder::KeyRef &JsonBuilder::KeyRef::operator=(double val)
{
  builder_->Pair(key_, val);
  return *this;
}
#endif

JsonBuilder::KeyRef &JsonBuilder::KeyRef::Null()
{
  builder_->Key(key_);
  builder_->Null();
  return *this;
}

JsonBuilder::ObjectScope JsonBuilder::KeyRef::Object()
{
  builder_->Key(key_);
  return ObjectScope(*builder_);
}

JsonBuilder::ArrayScope JsonBuilder::KeyRef::Array()
{
  builder_->Key(key_);
  return ArrayScope(*builder_);
}

JsonBuilder::ObjectScope::ObjectScope(JsonBuilder &builder)
  : builder_(&builder)
{
  builder_->BeginObject();
}

JsonBuilder::ObjectScope::ObjectScope(ObjectScope &&other) noexcept
  : builder_(other.builder_)
{
  other.builder_ = nullptr;
}

JsonBuilder::ObjectScope::~ObjectScope()
{
  if (builder_ != nullptr)
  {
    builder_->EndObject();
  }
}

JsonBuilder::KeyRef JsonBuilder::ObjectScope::operator[](std::string_view key)
{
  JSONLIB_ASSERT(builder_ != nullptr);
  return KeyRef(*builder_, key);
}

JsonBuilder::KeyRef JsonBuilder::ObjectScope::operator[](const char *key)
{
  JSONLIB_ASSERT((builder_ != nullptr) && (key != nullptr));
  return KeyRef(*builder_, std::string_view(key, strlen(key)));
}

JsonBuilder::ArrayScope::ArrayScope(JsonBuilder &builder)
  : builder_(&builder)
{
  builder_->BeginArray();
}

JsonBuilder::ArrayScope::ArrayScope(ArrayScope &&other) noexcept
  : builder_(other.builder_)
{
  other.builder_ = nullptr;
}

JsonBuilder::ArrayScope::~ArrayScope()
{
  if (builder_ != nullptr)
  {
    builder_->EndArray();
  }
}

JsonBuilder::ObjectScope JsonBuilder::ArrayScope::Object()
{
  JSONLIB_ASSERT(builder_ != nullptr);
  return ObjectScope(*builder_);
}

JsonBuilder::ArrayScope JsonBuilder::ArrayScope::Array()
{
  JSONLIB_ASSERT(builder_ != nullptr);
  return ArrayScope(*builder_);
}

}
