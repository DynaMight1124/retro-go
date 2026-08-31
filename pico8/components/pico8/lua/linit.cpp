/*
** $Id: linit.c,v 1.32 2011/04/08 19:17:36 roberto Exp $
** Initialization of libraries for lua.c and other clients
** See Copyright Notice in lua.h
*/


/*
** If you embed Lua in your program and need to open the standard
** libraries, call luaL_openlibs in your program. If you need a
** different set of libraries, copy this file to your project and edit
** it to suit your needs.
*/


#define linit_c
#define LUA_LIB

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"


/*
** these libs are loaded by lua.c and are readily available to any Lua
** program
*/
static const luaL_Reg loadedlibs[] = {
  {"_G", luaopen_base},
  {"_G", luaopen_pico8},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_STRLIBNAME, luaopen_string},
  // math: registered here so luaL_openlibs populates _G.math. The engines
  // embedded Lua stdlib prelude references math.pi, math.cos, math.floor
  // etc. via lmathlib.cpp's luaopen_math, which delegates to fix32's static
  // math members via l_mathop(z8::fix32::X). Without this line, the math
  // library is compiled in but never surfaced to _G, and any Lua code
  // touching math.* fails with "attempt to index global 'math' (a nil
  // value)". luaopen_math sets _G.math on requiref, so just listing it
  // here is enough.
  {LUA_MATHLIBNAME, luaopen_math},
  // PICO-8 does not expose desktop Lua's global `debug` library. Besides
  // leaking unsupported APIs, installing it changes ordinary cart semantics:
  // Golf Sunday uses an otherwise-uninitialised global named `debug` as its
  // private diagnostics flag, so a truthy library table enables collision
  // lines, [nil] labels and unclamped camera movement from the first frame.
  // Internal error reporting uses luaL_traceback() directly and does not need
  // this cart-visible table.
  {NULL, NULL}
};


/*
** these libs are preloaded and must be required before used
*/
static const luaL_Reg preloadedlibs[] = {
  {NULL, NULL}
};


LUALIB_API void luaL_openlibs (lua_State *L) {
  const luaL_Reg *lib;
  /* call open functions from 'loadedlibs' and set results to global table */
  for (lib = loadedlibs; lib->func; lib++) {
    luaL_requiref(L, lib->name, lib->func, 1);
    lua_pop(L, 1);  /* remove lib */
  }
  /* add open functions from 'preloadedlibs' into 'package.preload' table */
  luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");
  for (lib = preloadedlibs; lib->func; lib++) {
    lua_pushcfunction(L, lib->func);
    lua_setfield(L, -2, lib->name);
  }
  lua_pop(L, 1);  /* remove _PRELOAD table */
}

