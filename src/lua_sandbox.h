/******************************************************************************/
/** @file lua_sandbox.h
 *     No bytecode, no FFI and a smaller debug library for level and campaign Lua (this edition).
 * @par Comment:
 *     See lua_sandbox.c.
 * @par Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef LUA_SANDBOX_H
#define LUA_SANDBOX_H

#ifdef __cplusplus
extern "C" {
#endif

struct lua_State;

/** Take FFI away and make every loader a script can reach accept text only. Call once on a
 *  fresh state, after the standard libraries are open and before any script runs. */
void lua_sandbox_open(struct lua_State *L);

/** Leave scripts only debug.traceback and debug.getinfo. Call after the game's own Lua library
 *  (fxdata/lua/init.lua) has run, which keeps the parts it needs, and before campaign, map and
 *  mod scripts. */
void lua_sandbox_close_debug(struct lua_State *L);

/** Loads module name from file fname for require(), text only; a file of the game's own library
 *  asked for under another name after lua_sandbox_close_debug() gives the module already loaded.
 *  Leaves the loader (or an error message) on the stack, like luaL_loadfile(). */
int lua_sandbox_load_module(struct lua_State *L, const char *name, const char *fname);

/** luaL_loadfile() and luaL_dostring() for the engine's own calls, text only. */
int lua_sandbox_loadfile(struct lua_State *L, const char *fname);
int lua_sandbox_dostring(struct lua_State *L, const char *code);

#ifdef __cplusplus
}
#endif

#endif
