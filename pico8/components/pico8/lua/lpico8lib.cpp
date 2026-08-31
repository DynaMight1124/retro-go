//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2020 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//


#include <cmath>
#include <cctype>
#include <cstring>
#include <algorithm>

#define lpico8lib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "llimits.h"
#include "lobject.h"

typedef struct luaL_RegX {
  const char *name;
  void* func;
  const uint16_t tag;
} luaL_RegX;

const z8::fix32 TAU = 6.2831853071795864769252867665590057683936f;

static int pico8_foreach(lua_State* L) {
    // stack: 1 = table
    // stack: 2 = function
    //
    // Push another reference to the table on top of the stack (so we know
    // where it is, and this function can work for negative, positive and
    // pseudo indices
    lua_pushvalue(L, -2);
    // stack now contains: -1 => table; {-2 function; -3 table}
    lua_pushnil(L);
    // stack now contains: -1 => nil; -2 => table
    while (lua_next(L, -2))
    {
        // stack now contains: -1 => value; -2 => key; -3 => table
        lua_pushvalue(L, 2); // function
        // stack now contains: -1 function; => -2 => value; -3 => key; -4 => table
        // and i want key => value => function => table
        lua_pushnil(L);
        // stack now contains: -1 => nil; -2 => function; -3 => value; -4 => key; -5 => table
        lua_copy(L, -3, -1);
        // stack now contains: -1 => value; -2 => function; -3 => STALE; -4 => key; -5 => table
        lua_call(L, 1, 0); // 1 arg, 0 results
        // stack now contains: -1 => STALE; -2 => key; -3 => table
        lua_pop(L, 1); // pop stale value
    }
    // stack now contains: -1 => table (when lua_next returns 0 it pops the key
    // but does not push anything.)
    // Pop table
    lua_pop(L, 1);
    // Stack is now the same as it was on entry to this function
    return 0;
}
static int pico8_max(lua_State *l) {
    // Preserve the port's established nil-first compatibility behaviour
    // (needed by carts whose optional first value disappears mid-frame),
    // while using PICO-8's numeric coercion for booleans and numeric strings.
    if (lua_isnoneornil(l, 1)) {
        if (lua_gettop(l) >= 2) lua_pushvalue(l, 2);
        else lua_pushnil(l);
        return 1;
    }
    lua_pushnumber(l, lua_Number::max(lua_tonumber(l, 1), lua_tonumber(l, 2)));
    return 1;
}

static int pico8_min(lua_State *l) {
    if (lua_isnoneornil(l, 1)) {
        if (lua_gettop(l) >= 2) lua_pushvalue(l, 2);
        else lua_pushnil(l);
        return 1;
    }
    lua_pushnumber(l, lua_Number::min(lua_tonumber(l, 1), lua_tonumber(l, 2)));
    return 1;
}

static int pico8_mid(lua_State *l) {
    lua_Number x = lua_tonumber(l, 1);
    lua_Number y = lua_tonumber(l, 2);
    lua_Number z = lua_tonumber(l, 3);
    lua_pushnumber(l, x > y ? y > z ? y : lua_Number::min(x, z)
                            : x > z ? x : lua_Number::min(y, z));
    return 1;
}

static int pico8_ceil(lua_State *l) {
    lua_pushnumber(l, lua_Number::ceil(lua_tonumber(l, 1)));
    return 1;
}

static int pico8_flr(lua_State *l) {
    lua_pushnumber(l, lua_Number::floor(lua_tonumber(l, 1)));
    return 1;
}

static int pico8_cos(lua_State *l) {
    lua_pushnumber(l, cast_num(cosf(-TAU * lua_tonumber(l, 1))));
    return 1;
}

static int pico8_sin(lua_State *l) {
    lua_pushnumber(l, cast_num(z8::fix32::sin(-TAU * lua_tonumber(l, 1))));
    return 1;
}

static int pico8_atan2(lua_State *l) {
    lua_Number x = lua_tonumber(l, 1);
    lua_Number y = lua_tonumber(l, 2);
    // This could simply be atan2(-y,x) but since PICO-8 decided that
    // atan2(0,0) = 0.75 we need to do the same in our version.
    float a = 0.75 + atan2f(x, y) / TAU;
    lua_pushnumber(l, a >= 1 ? a - 1 : a);
    return 1;
}

static int pico8_sqrt(lua_State *l) {
    lua_Number x = lua_tonumber(l, 1);
    lua_pushnumber(l, cast_num(x.bits() >= 0 ? std::sqrt((float)x) : 0));
    return 1;
}

static int pico8_abs(lua_State *l) {
    lua_pushnumber(l, lua_Number::abs(lua_tonumber(l, 1)));
    return 1;
}

static int pico8_sgn(lua_State *l) {
    lua_pushnumber(l, cast_num(lua_tonumber(l, 1).bits() >= 0 ? 1.f : -1.f));
    return 1;
}

static int pico8_band(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) & lua_tonumber(l, 2));
    return 1;
}

static int pico8_bor(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) | lua_tonumber(l, 2));
    return 1;
}

static int pico8_bxor(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) ^ lua_tonumber(l, 2));
    return 1;
}

static int pico8_bnot(lua_State *l) {
    lua_pushnumber(l, ~lua_tonumber(l, 1));
    return 1;
}

static int pico8_shl(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) << int(lua_tonumber(l, 2)));
    return 1;
}

static int pico8_lshr(lua_State *l) {
    lua_pushnumber(l, lua_Number::lshr(lua_tonumber(l, 1), int(lua_tonumber(l, 2))));
    return 1;
}

static int pico8_shr(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) >> int(lua_tonumber(l, 2)));
    return 1;
}

static int pico8_rotl(lua_State *l) {
    lua_pushnumber(l, lua_Number::rotl(lua_tonumber(l, 1), int(lua_tonumber(l, 2))));
    return 1;
}

static int pico8_rotr(lua_State *l) {
    lua_pushnumber(l, lua_Number::rotr(lua_tonumber(l, 1), int(lua_tonumber(l, 2))));
    return 1;
}

static int pico8_tostr(lua_State *l) {
    char buffer[20];
    char const *s = buffer;
    auto hex = lua_toboolean(l, 2);
    switch (lua_type(l, 1))
    {
        case LUA_TNONE:
            // PICO-8 0.2.2 changelog: tostr() returns "" instead of nil
            buffer[0] = '\0';
            break;
        case LUA_TNUMBER: {
            lua_Number x = lua_tonumber(l, 1);
            if (hex) {
                uint32_t b = (uint32_t)x.bits();
                // b is uint32_t; the `>> 16` and `& 0xffff` operands promote
                // the result to `int` / `long unsigned int` on different
                // platforms, which `%x` rejects. Cast to `unsigned long` and
                // match the spec `%lx`. Same fix as the printf %u / cast
                // pattern we applied in pico8api.c.
                sprintf(buffer, "0x%04lx.%04lx",
                        (unsigned long)((b >> 16) & 0xfffful),
                        (unsigned long)(b & 0xfffful));
            } else {
                lua_number2str(buffer, x);
            }
            break;
        }
        case LUA_TSTRING:
            lua_pushvalue(l, 1);
            return 1;
        case LUA_TBOOLEAN: s = lua_toboolean(l, 1) ? "true" : "false"; break;
        case LUA_TTABLE:
            // PICO-8 0.1.12d changelog: “__tostring metatable method
            // observed by tostr() / print() / printh()”
            if (luaL_callmeta(l, 1, "__tostring")) {
                luaL_tolstring(l, 1, NULL);
                return 1;
            }
            [[fallthrough]];
        case LUA_TFUNCTION:
            // PICO-8 0.1.12d changelog: “tostr(x,true) can also be used to view
            // the hex value of functions and tables (uses Lua's tostring)”
            if (hex) {
                luaL_tolstring(l, 1, NULL);
                return 1;
            }
            [[fallthrough]];
        default: sprintf(buffer, "[%s]", luaL_typename(l, 1)); break;
    }
    lua_pushstring(l, s);
    return 1;
}

static int pico8_tonum(lua_State *l) {
    char const *s = lua_tostring(l, 1);
    lua_Number ret;
    // If parsing failed, PICO-8 returns nothing
    if (!luaO_str2d(s, strlen(s), &ret)) return 0;
    lua_pushnumber(l, ret);
    return 1;
}

static int pico8_chr(lua_State *l) {
    char s[2] = { (char)(uint8_t)lua_tonumber(l, 1), '\0' };
    lua_pushlstring(l, s, 1);
    return 1;
}

static int pico8_ord(lua_State *l) {
    size_t len;
    int n = 0;
    char const *s = luaL_checklstring(l, 1, &len);
    if (!lua_isnone(l, 2)) {
        if (!lua_isnumber(l, 2))
            return 0;
        n = int(lua_tonumber(l, 2)) - 1;
    }
    if (n < 0 || size_t(n) >= len)
        return 0;
    lua_pushnumber(l, uint8_t(s[n]));
    return 1;
}

static int pico8_split(lua_State *l) {
    size_t count = 0, hlen;
    char const *haystack = luaL_checklstring(l, 1, &hlen);
    if (!haystack)
        return 0;
    lua_newtable(l);
    // Split either by chunk size or by needle position
    int size = 0;
    char needle = ',';
    if (lua_isnumber(l, 2)) {
        size = int(lua_tonumber(l, 2));
        if (size <= 0)
            size = 1;
    } else if (lua_isstring(l, 2)) {
        needle = *lua_tostring(l, 2);
    }
    auto convert = lua_isnone(l, 3) || lua_toboolean(l, 3);
    char const *end = haystack + hlen + (!size && needle);
    for (char const *parser = haystack; parser < end; ) {
        lua_Number num;
        char const *next = size ? parser + size
                         : needle ? strchr(parser, needle) : parser + 1;
        if (!next || next > end)
            next = haystack + hlen;
        char saved = *next; // temporarily put a null terminator here
        *(char *)next = '\0';
        if (convert && luaO_str2d(parser, next - parser, &num))
            lua_pushnumber(l, num);
        else
            lua_pushstring(l, parser);
        *(char *)next = saved;
        lua_rawseti(l, -2, int(++count));
        parser = next + (!size && needle);
    }
    return 1;
}

static int fpcos(lua_State *l) {
    lua_pushnumber(l, z8::fix32::cos(lua_tonumber(l, 1)));
    return 1;
}

static int fpsin(lua_State *l) {
    lua_pushnumber(l, z8::fix32::sin(lua_tonumber(l, 1)));
    return 1;
}



static const luaL_RegX fast_pico8lib[] = {
  {"ceil",  NULL,	FCF_CEIL,   },
  {"flr",   NULL,	FCF_FLR,    },
  {"cos",   NULL,	FCF_COS,    },
  {"sin",   NULL,	FCF_SIN,    },
  {"sqrt",  NULL,	FCF_SQRT,   },
  {"abs",   NULL,	FCF_ABS,    },
  {"sgn",   NULL,	FCF_SGN,    },
  {"bnot",  NULL,	FCF_BNOT,   },

  //{"max",   (void*)pico8_max,	FCF_MAX,    },
  //{"min",   (void*)pico8_min,	FCF_MIN,    },
  //{"mid",   (void*)pico8_mid,	FCF_MID,    },
  //{"atan2", (void*)pico8_atan2, FCF_ATAN2	},
  //{"band",  (void*)pico8_band,	FCF_BAND,   },
  //{"bor",   (void*)pico8_bor,	FCF_BOR,    },
  //{"bxor",  (void*)pico8_bxor,	FCF_BXOR,   },
  //{"shl",   (void*)pico8_shl,	FCF_SHL,    },
  {"shr",   (void*)pico8_shr,	FCF_SHR,    },
  //{"lshr",  (void*)pico8_lshr,	FCF_LSHR,   },
  //{"rotl",  (void*)pico8_rotl,	FCF_ROTL,   },
  //{"rotr",  (void*)pico8_rotr,	FCF_ROTR,   },
  // Sentinel: luaL_RegX has 3 fields (name, func, tag); the third (tag) is
  // 0 here so the sentinel isn't a valid fast-call flag and lookups stop.
  {NULL, NULL, 0}
};

// regular call (non-fast) functions
// either they have >2 args or they don't return 1 value
static const luaL_Reg pico8lib[] = {
  {"approx_cos",       fpcos},
  {"approx_sin",       fpsin},

  {"slow_ceil",      pico8_ceil},
  {"slow_flr",       pico8_flr},
  {"slow_cos",       pico8_cos},
  {"slow_sin",       pico8_sin},
  {"slow_sqrt",      pico8_sqrt},
  {"slow_abs",       pico8_abs},
  {"slow_sgn",       pico8_sgn},
  {"slow_bnot",      pico8_bnot},
//
  {"max",       pico8_max},
  {"min",       pico8_min},
  {"mid",       pico8_mid},
  {"atan2",     pico8_atan2},
  {"band",      pico8_band},
  {"bor",       pico8_bor},
  {"bxor",      pico8_bxor},
  {"shl",       pico8_shl},
//  {"shr",       pico8_shr},
  {"lshr",      pico8_lshr},
  {"rotl",      pico8_rotl},
  {"rotr",      pico8_rotr},
// non-number // complicated (also don't need to be faster)
  {"tostr",     pico8_tostr},
  {"tonum",     pico8_tonum},
  {"chr",       pico8_chr},
  {"ord",       pico8_ord},
  {"split",     pico8_split},
  {"foreach",   pico8_foreach},
  {NULL, NULL}
};


/*
** Register PICO-8 functions in global table
*/
LUAMOD_API int luaopen_pico8 (lua_State *L) {
  lua_pushglobaltable(L);
  luaL_setfuncs(L, pico8lib, 0);
  const luaL_RegX* l = fast_pico8lib;
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
      lua_pushcfastcall(L, NULL, l->tag);
      lua_setglobal(L, l->name);
  }
  return 1;
}
