/* Generated from tools/psp/recipes.json; contains no game data. */
#pragma once
#include <stdint.h>
#include <stddef.h>
typedef struct { uint32_t dst, size, src, extra; uint16_t kind, xform; } mk64_recipe_t;
typedef struct { uint8_t nwords, shapes[16]; } mk64_pattern_t;
extern const mk64_recipe_t mk64_recipes[];
extern const size_t mk64_recipe_count;
extern const uint16_t mk64_recipe_dst_order[];
extern const mk64_pattern_t mk64_patterns[];
extern const size_t mk64_pattern_count;
typedef struct { uint32_t rom_off, rom_len, unpacked_len, packed_off; } mk64_course_dl_t;
extern const mk64_course_dl_t mk64_course_displaylists[];
extern const size_t mk64_course_displaylist_count;
