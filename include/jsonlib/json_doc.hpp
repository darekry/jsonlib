// SPDX-License-Identifier: MIT
#ifndef JSONLIB_JSON_DOC_HPP
#define JSONLIB_JSON_DOC_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "jsonlib/config.hpp"
#include "jsonlib/json_reader.hpp"
#include "jsonlib/json_span_reader.hpp"

namespace jsonlib {

enum class JsonBackend : uint8_t
{
  None = 0,
  TokenArray,
  Span,
};

class JsonDoc;
class JsonArrayCursor;
class JsonObjectCursor;

/* Facade over one parsed document. Token backend walks the token pool
 * (helpers from json_reader.hpp); span backend walks the validated
 * buffer directly. Use JsonDoc + JsonValue + cursors - to jedyne API readera. */
class JsonValue
{
public:
  JsonValue() = default;

  bool Exists() const;
  bool IsNull() const;
  bool IsObject() const;
  bool IsArray() const;
  bool IsString() const;
  bool IsPrimitive() const;

  explicit operator bool() const;

  std::string_view Raw() const;
  size_t Count() const;

  std::optional<std::string_view> Str() const;
  std::optional<int32_t> Int() const;
#if !JSONLIB_NO_FLOAT
  std::optional<float> Float() const;
#endif
  std::optional<bool> Bool() const;
  bool Ip(uint8_t out[4]) const;

  JsonValue operator[](std::string_view key) const;
  JsonValue operator[](size_t idx) const;

  bool operator==(std::string_view val) const;
  bool operator!=(std::string_view val) const { return !(*this == val); }

private:
  friend class JsonDoc;
  friend class JsonArrayCursor;
  friend class JsonObjectCursor;

#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  JsonValue(const JsonDoc *doc, int tok) : doc_(doc), tok_(tok) {}

  const JsonDoc *doc_ = nullptr;
  int tok_ = -1;
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  explicit JsonValue(JsonSpan span) : span_(span) {}

  JsonSpan span_{};
#endif
};

class JsonArrayCursor
{
public:
  explicit JsonArrayCursor(JsonValue array);

  bool Next(JsonValue &out);

private:
  JsonValue array_;
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  int next_tok_ = -1;
  int end_ = -1;
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  const char *pos_ = nullptr;
#endif
};

class JsonObjectCursor
{
public:
  explicit JsonObjectCursor(JsonValue object);

  bool Next(std::string_view &key, JsonValue &out);

private:
  JsonValue object_;
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  int next_tok_ = -1;
  int end_ = -1;
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  const char *pos_ = nullptr;
#endif
};

class JsonDoc
{
public:
  JsonDoc() = default;
  JsonDoc(const JsonDoc &) = delete;
  JsonDoc &operator=(const JsonDoc &) = delete;

  bool Parse(const char *json, size_t len);

  JsonValue root() const;
  JsonReadError error() const { return error_; }
  JsonBackend backend() const { return backend_; }
  int token_count() const { return token_count_; }

private:
  friend class JsonValue;
  friend class JsonArrayCursor;
  friend class JsonObjectCursor;

#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  Token tokens_[JSONLIB_TOKEN_CAP];
  const char *buf_ = nullptr;
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  JsonSpan root_span_{};
#endif
  JsonReadError error_ = JsonReadError::None;
  JsonBackend backend_ = JsonBackend::None;
  int token_count_ = 0;
};

}

#endif
