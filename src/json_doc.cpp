// SPDX-License-Identifier: MIT
#include "jsonlib/json_doc.hpp"

namespace jsonlib {

static_assert(sizeof(Token) == 8u,
              "compact jsmn token expected (JSMN_PARENT_LINKS unsupported)");

namespace {

#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
bool ValidateTokens(const char *json, size_t len, const Token *tokens,
                    int count, JsonReadError &err)
{
  if (count <= 0)
  {
    err = JsonReadError::Syntax;
    return false;
  }

  size_t trimmed = len;
  while ((trimmed > 0u) && detail::IsWhitespace(json[trimmed - 1u]))
  {
    trimmed--;
  }

  const Token &root = tokens[0];
  if ((root.start < 0) || (root.end < root.start))
  {
    err = JsonReadError::Syntax;
    return false;
  }
  size_t root_end = (size_t)root.end;
  if (root.type == JSMN_STRING)
  {
    root_end++;
  }
  if (root_end != trimmed)
  {
    err = JsonReadError::Syntax;
    return false;
  }

  /* Pozycje koncow otwartych kontenerow; czytane tylko po zapisie
   * (guard top > 0), wiec bez kosztownego zerowania. int16_t, bo tokeny
   * i tak nie siegaja dalej niz 32767. */
  int16_t stack[JSONLIB_JSON_MAX_DEPTH + 1];
  int top = 0;
  for (int i = 0; i < count; i++)
  {
    const Token &tok = tokens[i];
    if ((tok.start < 0) || (tok.end < tok.start) ||
        ((size_t)tok.end > len))
    {
      err = JsonReadError::Syntax;
      return false;
    }
    if (tok.type == JSMN_PRIMITIVE)
    {
      const std::string_view raw(json + tok.start,
                                 (size_t)(tok.end - tok.start));
      if (!detail::ValidatePrimitive(raw))
      {
        err = JsonReadError::Syntax;
        return false;
      }
    }
    else if (tok.type == JSMN_STRING)
    {
      if (!detail::ValidateStringContent(json + tok.start, json + tok.end))
      {
        err = JsonReadError::Syntax;
        return false;
      }
    }
    if ((tok.type == JSMN_OBJECT) || (tok.type == JSMN_ARRAY))
    {
      while ((top > 0) && (stack[top - 1] <= tok.start))
      {
        top--;
      }
      const int depth = top + 1;
      if (depth > JSONLIB_JSON_MAX_DEPTH)
      {
        err = JsonReadError::TooDeep;
        return false;
      }
      stack[top] = tok.end;
      top++;
    }
  }

  err = JsonReadError::None;
  return true;
}
#endif

}

bool JsonDoc::Parse(const char *json, size_t len)
{
  error_ = JsonReadError::None;
  backend_ = JsonBackend::None;
  token_count_ = 0;
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  buf_ = nullptr;
#endif

  if (json == nullptr)
  {
    error_ = JsonReadError::Syntax;
    return false;
  }

#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  {
    const int n = JsonParseTokens(json, len, tokens_, JSONLIB_TOKEN_CAP);
    if (n > 0)
    {
      JsonReadError verr = JsonReadError::None;
      if (!ValidateTokens(json, len, tokens_, n, verr))
      {
        error_ = verr;
        return false;
      }
      token_count_ = n;
      buf_ = json;
      backend_ = JsonBackend::TokenArray;
      return true;
    }
    if (n == JSMN_ERROR_NOMEM)
    {
#if JSONLIB_READER_MODE == JSONLIB_READER_HYBRID
      JsonReadError serr = JsonReadError::None;
      JsonSpan root;
      if (detail::ParseDocument(json, len, root, serr))
      {
        root_span_ = root;
        backend_ = JsonBackend::Span;
        return true;
      }
      error_ = serr;
      return false;
#else
      error_ = JsonReadError::NoMemory;
      return false;
#endif
    }
    error_ = ((n == JSMN_ERROR_INVAL) || (n == 0)) ? JsonReadError::Syntax
                                                      : JsonReadError::Partial;
    return false;
  }
#else
  {
    JsonReadError serr = JsonReadError::None;
    JsonSpan root;
    if (!detail::ParseDocument(json, len, root, serr))
    {
      error_ = serr;
      return false;
    }
    root_span_ = root;
    backend_ = JsonBackend::Span;
    return true;
  }
#endif
}

JsonValue JsonDoc::root() const
{
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if (backend_ == JsonBackend::TokenArray)
  {
    return JsonValue(this, 0);
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  if (backend_ == JsonBackend::Span)
  {
    return JsonValue(root_span_);
  }
#endif
  return JsonValue{};
}

bool JsonValue::Exists() const
{
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if (tok_ >= 0)
  {
    return (doc_ != nullptr) &&
           JsonTokenContains(doc_->token_count_, tok_);
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  return span_.valid();
#else
  return false;
#endif
}

bool JsonValue::IsNull() const
{
  return IsPrimitive() && (Raw() == "null");
}

bool JsonValue::IsObject() const
{
  if (!Exists())
  {
    return false;
  }
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if (tok_ >= 0)
  {
    return (doc_ != nullptr) &&
           JsonTokenContains(doc_->token_count_, tok_) &&
           (doc_->tokens_[tok_].type == JSMN_OBJECT);
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  return span_.type == JsonSpanType::Object;
#else
  return false;
#endif
}

bool JsonValue::IsArray() const
{
  if (!Exists())
  {
    return false;
  }
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if (tok_ >= 0)
  {
    return (doc_ != nullptr) &&
           JsonTokenContains(doc_->token_count_, tok_) &&
           (doc_->tokens_[tok_].type == JSMN_ARRAY);
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  return span_.type == JsonSpanType::Array;
#else
  return false;
#endif
}

bool JsonValue::IsString() const
{
  if (!Exists())
  {
    return false;
  }
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if (tok_ >= 0)
  {
    return (doc_ != nullptr) &&
           JsonTokenContains(doc_->token_count_, tok_) &&
           (doc_->tokens_[tok_].type == JSMN_STRING);
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  return span_.type == JsonSpanType::String;
#else
  return false;
#endif
}

bool JsonValue::IsPrimitive() const
{
  if (!Exists())
  {
    return false;
  }
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if (tok_ >= 0)
  {
    return (doc_ != nullptr) &&
           JsonTokenContains(doc_->token_count_, tok_) &&
           (doc_->tokens_[tok_].type == JSMN_PRIMITIVE);
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  return span_.type == JsonSpanType::Primitive;
#else
  return false;
#endif
}

JsonValue::operator bool() const
{
  return Exists() && !IsNull();
}

std::string_view JsonValue::Raw() const
{
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if (tok_ >= 0)
  {
    return (doc_ != nullptr) ? JsonTokenView(doc_->buf_, doc_->tokens_,
                                             doc_->token_count_, tok_)
                             : std::string_view{};
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  return span_.content();
#else
  return {};
#endif
}

size_t JsonValue::Count() const
{
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if (tok_ >= 0)
  {
    if (!IsObject() && !IsArray())
    {
      return 0u;
    }
    const int count = doc_->tokens_[tok_].size;
      return (count > 0) ? (size_t)count : 0u;
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  if (IsObject() || IsArray())
  {
    return detail::CountEntries(span_);
  }
#endif
  return 0u;
}

std::optional<std::string_view> JsonValue::Str() const
{
  if (!IsString())
  {
    return std::nullopt;
  }
  return Raw();
}

std::optional<int32_t> JsonValue::Int() const
{
  if (!IsPrimitive())
  {
    return std::nullopt;
  }
  int32_t val = 0;
  if (!JsonParseInt(Raw(), val))
  {
    return std::nullopt;
  }
  return val;
}

#if !JSONLIB_NO_FLOAT
std::optional<float> JsonValue::Float() const
{
  if (!IsPrimitive())
  {
    return std::nullopt;
  }
  float val = 0.0f;
  if (!JsonParseFloat(Raw(), val))
  {
    return std::nullopt;
  }
  return val;
}
#endif

std::optional<bool> JsonValue::Bool() const
{
  if (!IsPrimitive())
  {
    return std::nullopt;
  }
  bool val = false;
  if (!JsonParseBool(Raw(), val))
  {
    return std::nullopt;
  }
  return val;
}

bool JsonValue::Ip(uint8_t out[4]) const
{
  return IsString() && JsonParseIp(Raw(), out);
}

JsonValue JsonValue::operator[](std::string_view key) const
{
  if (!IsObject())
  {
    return JsonValue{};
  }
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if (tok_ >= 0)
  {
    return JsonValue(doc_, JsonTokenFind(doc_->buf_, doc_->tokens_,
                                         doc_->token_count_, tok_, key));
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  JsonSpan value;
  if (detail::FindMember(span_, key, value))
  {
    return JsonValue(value);
  }
#endif
  return JsonValue{};
}

JsonValue JsonValue::operator[](size_t idx) const
{
  if (!IsArray())
  {
    return JsonValue{};
  }
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if ((tok_ >= 0) && (doc_ != nullptr) &&
      JsonTokenContains(doc_->token_count_, tok_))
  {
    const int count = doc_->tokens_[tok_].size;
    if ((count <= 0) || (idx >= (size_t)count))
    {
      return JsonValue{};
    }
    int tok = tok_ + 1;
    for (size_t i = 0u; i < idx; i++)
    {
      tok = JsonTokenSkip(doc_->tokens_, doc_->token_count_, tok);
      if (tok < 0)
      {
        return JsonValue{};
      }
    }
    return JsonValue(doc_, tok);
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  JsonSpan value;
  if (detail::FindElement(span_, idx, value))
  {
    return JsonValue(value);
  }
#endif
  return JsonValue{};
}

bool JsonValue::operator==(std::string_view val) const
{
  return Exists() && (Raw() == val);
}

JsonArrayCursor::JsonArrayCursor(JsonValue array) : array_(array)
{
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if (array_.IsArray() && (array_.tok_ >= 0) && (array_.doc_ != nullptr))
  {
    next_tok_ = array_.tok_ + 1;
    end_ = array_.doc_->tokens_[array_.tok_].end;
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
#if JSONLIB_READER_MODE == JSONLIB_READER_HYBRID
  if (array_.IsArray() && (array_.tok_ < 0) && array_.span_.valid())
#else
  if (array_.IsArray() && array_.span_.valid())
#endif
  {
    pos_ = array_.span_.begin + 1;
  }
#endif
}

bool JsonArrayCursor::Next(JsonValue &out)
{
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if ((array_.tok_ >= 0) && (array_.doc_ != nullptr))
  {
    if (next_tok_ < 0)
    {
      return false;
    }
    const Token *toks = array_.doc_->tokens_;
    const int count = array_.doc_->token_count_;
    if (!JsonTokenContains(count, next_tok_) || (toks[next_tok_].start >= end_))
    {
      next_tok_ = -1;
      return false;
    }
    out = JsonValue(array_.doc_, next_tok_);
    next_tok_ = JsonTokenSkip(toks, count, next_tok_);
    return true;
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  if (array_.span_.valid())
  {
    JsonSpan value;
    if (!detail::NextElement(array_.span_, pos_, value))
    {
      return false;
    }
    out = JsonValue(value);
    return true;
  }
#endif
  return false;
}

JsonObjectCursor::JsonObjectCursor(JsonValue object) : object_(object)
{
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if (object_.IsObject() && (object_.tok_ >= 0) && (object_.doc_ != nullptr))
  {
    next_tok_ = object_.tok_ + 1;
    end_ = object_.doc_->tokens_[object_.tok_].end;
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
#if JSONLIB_READER_MODE == JSONLIB_READER_HYBRID
  if (object_.IsObject() && (object_.tok_ < 0) && object_.span_.valid())
#else
  if (object_.IsObject() && object_.span_.valid())
#endif
  {
    pos_ = object_.span_.begin + 1;
  }
#endif
}

bool JsonObjectCursor::Next(std::string_view &key, JsonValue &out)
{
#if JSONLIB_READER_MODE != JSONLIB_READER_SPAN
  if ((object_.tok_ >= 0) && (object_.doc_ != nullptr))
  {
    if (next_tok_ < 0)
    {
      return false;
    }
    const char *buf = object_.doc_->buf_;
    const Token *toks = object_.doc_->tokens_;
    const int count = object_.doc_->token_count_;
    if (!JsonTokenContains(count, next_tok_))
    {
      next_tok_ = -1;
      return false;
    }
    const Token &key_tok = toks[next_tok_];
    if ((key_tok.type != JSMN_STRING) || (key_tok.start >= end_))
    {
      next_tok_ = -1;
      return false;
    }
    const int value_tok = next_tok_ + 1;
    if (!JsonTokenContains(count, value_tok) ||
        (toks[value_tok].start >= end_))
    {
      next_tok_ = -1;
      return false;
    }
    key = JsonTokenView(buf, toks, count, next_tok_);
    out = JsonValue(object_.doc_, value_tok);
    next_tok_ = JsonTokenSkip(toks, count, value_tok);
    return true;
  }
#endif
#if JSONLIB_READER_MODE != JSONLIB_READER_TOKEN
  if (object_.span_.valid())
  {
    JsonSpan value;
    if (!detail::NextMember(object_.span_, pos_, key, value))
    {
      return false;
    }
    out = JsonValue(value);
    return true;
  }
#endif
  return false;
}

}
