// SPDX-License-Identifier: MIT
#ifndef JSONLIB_JSON_WRITER_HPP
#define JSONLIB_JSON_WRITER_HPP

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <type_traits>
#include <utility>

#include "jsonlib/config.hpp"

namespace jsonlib {

namespace detail {

template <typename Buffer, typename = void>
struct IsCharBuffer : std::false_type
{
};

template <typename Buffer>
struct IsCharBuffer<Buffer,
                    std::void_t<decltype(std::data(std::declval<Buffer &>())),
                                decltype(std::size(std::declval<Buffer &>()))>>
  : std::bool_constant<
      std::is_convertible_v<decltype(std::data(std::declval<Buffer &>())),
                            char *> &&
      std::is_convertible_v<decltype(std::size(std::declval<Buffer &>())),
                            size_t>>
{
};

}

class JsonBuilder
{
public:
  class KeyRef;
  class ObjectScope;
  class ArrayScope;

  static constexpr size_t kMaxDepth = JSONLIB_JSON_MAX_DEPTH;

  JsonBuilder(char *buf, size_t cap);

  /* Non-owning adapters for contiguous mutable char buffers:
   * std::array<char, N>, std::vector<char>, std::span<char> (C++20) and
   * plain C arrays. The buffer must outlive the builder and must not be
   * resized, reallocated or moved while the builder is in use. The usable
   * capacity is std::size(buf), so pre-size vectors/strings first. */
  template <typename Buffer,
            typename = std::enable_if_t<detail::IsCharBuffer<Buffer>::value>>
  explicit JsonBuilder(Buffer &buf)
    : JsonBuilder(std::data(buf), (size_t)std::size(buf))
  {
  }

  /* Accepts temporary views such as std::span<char>(tx); containers with
   * const data (std::vector<char> const&, string_view, etc.) are rejected. */
  template <typename Buffer,
            typename =
              std::enable_if_t<detail::IsCharBuffer<const Buffer>::value>>
  explicit JsonBuilder(const Buffer &buf)
    : JsonBuilder(std::data(buf), (size_t)std::size(buf))
  {
  }

  JsonBuilder &BeginObject();
  JsonBuilder &EndObject();
  JsonBuilder &BeginArray();
  JsonBuilder &EndArray();

  JsonBuilder &Key(std::string_view key);
  JsonBuilder &Key(const char *key);

  JsonBuilder &Val(std::string_view val);
  JsonBuilder &Val(const char *val);
  JsonBuilder &Val(int32_t val);
  JsonBuilder &Val(uint32_t val);
  JsonBuilder &Val(bool val);
#if !JSONLIB_NO_FLOAT
  JsonBuilder &Val(float val);
  JsonBuilder &Val(double val);
#else
  /* NO_FLOAT: floaty celowo skasowane - czytelny blad kompilacji
   * zamiast cichego +3.5 KB flash (snprintf %g). */
  JsonBuilder &Val(float) = delete;
  JsonBuilder &Val(double) = delete;
#endif

  /* int literals are int, which is not necessarily int32_t (on ARM GCC
   * int32_t is long). This template gives them an exact match instead of
   * relying on an ambiguous integral conversion. */
  template <typename T,
            typename =
              std::enable_if_t<std::is_integral_v<T> &&
                               !std::is_same_v<T, bool>>>
  JsonBuilder &Val(T val)
  {
    static_assert(sizeof(T) <= sizeof(int32_t),
                  "jsonlib supports 32-bit integers; cast explicitly");
    if constexpr (std::is_signed_v<T>)
    {
      return Val(static_cast<int32_t>(val));
    }
    else
    {
      return Val(static_cast<uint32_t>(val));
    }
  }

  JsonBuilder &Null();

  ObjectScope Object();
  ArrayScope Array();

  /* Verbatim, BEZ logiki przecinków. Tylko escape hatch. */
  JsonBuilder &Raw(std::string_view frag);

  JsonBuilder &Pair(std::string_view key, std::string_view val);
  JsonBuilder &Pair(std::string_view key, const char *val);
  JsonBuilder &Pair(const char *key, std::string_view val);
  JsonBuilder &Pair(const char *key, const char *val);
  JsonBuilder &Pair(std::string_view key, int32_t val);
  JsonBuilder &Pair(const char *key, int32_t val);
  JsonBuilder &Pair(std::string_view key, uint32_t val);
  JsonBuilder &Pair(const char *key, uint32_t val);
  JsonBuilder &Pair(std::string_view key, bool val);
  JsonBuilder &Pair(const char *key, bool val);
#if !JSONLIB_NO_FLOAT
  JsonBuilder &Pair(std::string_view key, float val);
  JsonBuilder &Pair(const char *key, float val);
  JsonBuilder &Pair(std::string_view key, double val);
  JsonBuilder &Pair(const char *key, double val);
#else
  JsonBuilder &Pair(std::string_view key, float) = delete;
  JsonBuilder &Pair(std::string_view key, double) = delete;
  JsonBuilder &Pair(const char *key, float) = delete;
  JsonBuilder &Pair(const char *key, double) = delete;
#endif

  template <typename T,
            typename =
              std::enable_if_t<std::is_integral_v<T> &&
                               !std::is_same_v<T, bool>>>
  JsonBuilder &Pair(std::string_view key, T val)
  {
    Key(key);
    Val(val);
    return *this;
  }

  template <typename T,
            typename =
              std::enable_if_t<std::is_integral_v<T> &&
                               !std::is_same_v<T, bool>>>
  JsonBuilder &Pair(const char *key, T val)
  {
    Key(key);
    Val(val);
    return *this;
  }

  KeyRef operator[](std::string_view key);
  KeyRef operator[](const char *key);

  void Fail();

  bool ok() const;
  size_t len() const;
  const char *c_str() const;

private:
  void Reset(char *buf, size_t cap);
  void BeforeKey();
  void BeforeValue();
  void AfterValue();
  void AppendChar(char c);
  void AppendRaw(const char *p, size_t n);
  void AppendQuoted(std::string_view s);

  static constexpr uint8_t kFirst = 0x80u;
  static constexpr uint8_t kTypeMask = 0x7Fu;

  char *buf_;
  size_t cap_;
  size_t len_;

  /* Stan poziomu w JEDNYM bajcie: bit 7 = pierwszy element,
   * bity 0-6 = 'O' (obiekt) / 'A' (tablica). 8 B zamiast 16 B. */
  uint8_t st_[kMaxDepth];
  uint8_t depth_;
  bool pending_key_;
  bool ok_;
};

class JsonBuilder::KeyRef
{
public:
  KeyRef(JsonBuilder &builder, std::string_view key)
    : builder_(&builder), key_(key)
  {
  }
  KeyRef(const KeyRef &) = default;
  KeyRef(KeyRef &&) = default;
  KeyRef &operator=(const KeyRef &) = delete;
  KeyRef &operator=(KeyRef &&) = delete;

  KeyRef &operator=(std::string_view val);
  KeyRef &operator=(const char *val);
  KeyRef &operator=(int32_t val);
  KeyRef &operator=(uint32_t val);
  KeyRef &operator=(bool val);
#if !JSONLIB_NO_FLOAT
  KeyRef &operator=(float val);
  KeyRef &operator=(double val);
#else
  KeyRef &operator=(float) = delete;
  KeyRef &operator=(double) = delete;
#endif

  template <typename T,
            typename =std::enable_if_t<std::is_integral_v<T> &&!std::is_same_v<T, bool>>>
  KeyRef &operator=(T val)
  {
    static_assert(sizeof(T) <= sizeof(int32_t),
                  "jsonlib supports 32-bit integers; cast explicitly");
    if constexpr (std::is_signed_v<T>)
    {
      builder_->Pair(key_, static_cast<int32_t>(val));
    }
    else
    {
      builder_->Pair(key_, static_cast<uint32_t>(val));
    }
    return *this;
  }

  KeyRef &Null();

  ObjectScope Object();
  ArrayScope Array();

private:
  JsonBuilder *builder_;
  std::string_view key_;
};

class JsonBuilder::ObjectScope
{
public:
  explicit ObjectScope(JsonBuilder &builder);
  ObjectScope(const ObjectScope &) = delete;
  ObjectScope(ObjectScope &&other) noexcept;
  ObjectScope &operator=(const ObjectScope &) = delete;
  ObjectScope &operator=(ObjectScope &&) = delete;
  ~ObjectScope();

  KeyRef operator[](std::string_view key);
  KeyRef operator[](const char *key);

private:
  JsonBuilder *builder_;
};

class JsonBuilder::ArrayScope
{
public:
  explicit ArrayScope(JsonBuilder &builder);
  ArrayScope(const ArrayScope &) = delete;
  ArrayScope(ArrayScope &&other) noexcept;
  ArrayScope &operator=(const ArrayScope &) = delete;
  ArrayScope &operator=(ArrayScope &&) = delete;
  ~ArrayScope();

  ObjectScope Object();
  ArrayScope Array();

private:
  JsonBuilder *builder_;
};

}

#endif
