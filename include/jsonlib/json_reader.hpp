// SPDX-License-Identifier: MIT
#ifndef JSONLIB_JSON_READER_HPP
#define JSONLIB_JSON_READER_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "jsonlib/config.hpp"

#ifndef JSMN_HEADER
#define JSMN_HEADER
#define JSONLIB_UNDEF_JSMN_HEADER
#endif
#include "jsmn.h"
#ifdef JSONLIB_UNDEF_JSMN_HEADER
#undef JSMN_HEADER
#undef JSONLIB_UNDEF_JSMN_HEADER
#endif

namespace jsonlib {

using Token = jsmntok_t;

/* Niskopoziomowy backend tokenowy (uzytek wewnetrzny JsonDoc).
 * Zwraca liczbe tokenow albo ujemny kod bledu jsmn (JSMN_ERROR_*).
 * Pusty/niepelny input: 0 albo JSMN_ERROR_PART - oba oznaczaja blad,
 * JsonDoc mapuje je na JsonReadError::Syntax/Partial. */
int JsonParseTokens(const char *json, size_t len, Token *tokens, int token_cap);

/* Nawigacja po puli tokenow (bez alokacji, bez wyjatkow).
 * Nieprawidlowy indeks = -1. Funkcje same sprawdzaja zakresy. */
bool JsonTokenContains(int count, int tok);
int JsonTokenSkip(const Token *toks, int count, int tok);
int JsonTokenFind(const char *buf, const Token *toks, int count, int obj,
                  std::string_view key);
std::string_view JsonTokenView(const char *buf, const Token *toks, int count,
                               int tok);

bool JsonParseInt(std::string_view s, int32_t &out);
#if !JSONLIB_NO_FLOAT
bool JsonParseFloat(std::string_view s, float &out);
#endif
bool JsonParseBool(std::string_view s, bool &out);
bool JsonParseIp(std::string_view s, uint8_t out[4]);

void JsonCopyStr(char *dst, size_t cap, std::string_view sv);

}

#endif
