// SPDX-License-Identifier: MIT
#ifndef JSONLIB_CONFIG_HPP
#define JSONLIB_CONFIG_HPP

#ifndef JSONLIB_ASSERT
#define JSONLIB_ASSERT(expr) ((void)0)
#endif

/* Reader backends. All translation units must see the same mode. */
#define JSONLIB_READER_TOKEN  0
#define JSONLIB_READER_SPAN   1
#define JSONLIB_READER_HYBRID 2

#ifndef JSONLIB_READER_MODE
#define JSONLIB_READER_MODE JSONLIB_READER_HYBRID
#endif

#if (JSONLIB_READER_MODE < JSONLIB_READER_TOKEN) || \
    (JSONLIB_READER_MODE > JSONLIB_READER_HYBRID)
#error invalid JSONLIB_READER_MODE
#endif

/* Maximum accepted container nesting (root object/array counts as level 1). */
#ifndef JSONLIB_JSON_MAX_DEPTH
#define JSONLIB_JSON_MAX_DEPTH 8
#endif

#if (JSONLIB_READER_MODE == JSONLIB_READER_TOKEN) || \
    (JSONLIB_READER_MODE == JSONLIB_READER_HYBRID)
#ifndef JSONLIB_TOKEN_CAP
#if JSONLIB_READER_MODE == JSONLIB_READER_TOKEN
#define JSONLIB_TOKEN_CAP 320
#else
#define JSONLIB_TOKEN_CAP 128
#endif
#endif
#endif

#endif
