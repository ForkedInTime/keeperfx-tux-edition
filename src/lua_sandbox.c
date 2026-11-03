/******************************************************************************/
/** @file lua_sandbox.c
 *     No bytecode, no FFI and a smaller debug library for level and campaign Lua (this edition).
 * @par Purpose:
 *     Level, campaign and mod scripts arrive with downloaded maps and Workshop items, and
 *     lua_readonly_io.c already keeps them from writing files. Three routes past that were left
 *     (upstream issue #5567 names the first):
 *     - FFI. The engine never opens LuaJIT's ffi library, but LuaJIT loads it on demand the moment
 *       it parses a 64-bit integer literal such as 1LL, and registers it as package.loaded.ffi,
 *       so `local _ = 1LL; local ffi = require "ffi"` called any C function. It is loaded once
 *       here, while nothing else runs, and the module is taken away: the literal support it
 *       set up stays, so 1LL keeps working and does not register the module again, and nothing
 *       else holds the module (checked: it is garbage-collected).
 *     - Bytecode. LuaJIT runs a precompiled chunk without checking it, so a crafted one can read
 *       and write any memory. Chunks came in through loadstring and load, through loadfile,
 *       dofile and require (a map's .lua file may be bytecode), and through the game's own
 *       serialisation, which rebuilt saved functions from bytecode taken from a saved game, a
 *       multiplayer resync sent by another player, or the Game table a map had filled. Every
 *       loader a script can reach is replaced here by one that accepts text only, the engine's
 *       own loads (lua_base.c) go through lua_sandbox_loadfile/lua_sandbox_dostring, and
 *       serialisation now saves functions as source text (fxdata/lua/core/serialisation.lua).
 *       The replacements are new C functions, not wrappers, so no original loader survives for
 *       anything to reach.
 *     - The debug library: debug.getregistry, getupvalue and setupvalue reach whatever the game's
 *       own Lua holds. Scripts that load after the game's library keep debug.traceback and
 *       debug.getinfo, the parts the shipped campaigns use; the library modules keep their own
 *       copy, captured when they load. A campaign can supply a module of the same name that the
 *       game's init.lua then loads in its place, before the debug library is cut down; that
 *       module still loads no bytecode and reaches no FFI, which is what the cut-down debug
 *       library is a second line of defence for.
 * @par Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"

#include <lua.h>
#include <lauxlib.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua_sandbox.h"
#include "bflib_basics.h"
#include "globals.h"

#include "post_inc.h"

/******************************************************************************/

static const char sb_binary_refused[] = "attempt to load a binary chunk (bytecode is not accepted)";

/** Registry keys: the game's own library files (real path -> module name), and whether the
 *  debug library has been cut down for the scripts that load after it. */
#define SB_LIBRARY_KEY "KeeperFX.sandbox.library"
#define SB_CLOSED_KEY "KeeperFX.sandbox.closed"

/** A load's result as Lua's own loaders return it: the function, or nil and the message. */
static int sb_load_result(lua_State *L, int status)
{
    if (status == 0)
        return 1;
    lua_pushnil(L);
    lua_insert(L, -2);
    return 2;
}

/** The optional mode argument at idx: true unless it rules out text. */
static int sb_mode_allows_text(lua_State *L, int idx)
{
    const char *mode = luaL_optstring(L, idx, "bt");
    return strchr(mode, 't') != NULL;
}

/** The optional env argument at idx becomes the environment of the function on top. */
static void sb_set_env(lua_State *L, int idx)
{
    if (!lua_isnoneornil(L, idx))
    {
        lua_pushvalue(L, idx);
        lua_setfenv(L, -2);
    }
}

/** loadstring(s [, chunkname]) */
static int sb_loadstring(lua_State *L)
{
    size_t len;
    const char *s = luaL_checklstring(L, 1, &len);
    const char *chunkname = luaL_optstring(L, 2, s);
    return sb_load_result(L, luaL_loadbufferx(L, s, len, chunkname, "t"));
}

/** load(chunk [, chunkname [, mode [, env]]]); chunk is a string or a function returning pieces */
static int sb_load(lua_State *L)
{
    if (!sb_mode_allows_text(L, 3))
    {
        lua_pushnil(L);
        lua_pushstring(L, sb_binary_refused);
        return 2;
    }
    int status;
    if (lua_type(L, 1) == LUA_TSTRING)
    {
        size_t len;
        const char *s = lua_tolstring(L, 1, &len);
        status = luaL_loadbufferx(L, s, len, luaL_optstring(L, 2, s), "t");
    } else
    {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        const char *chunkname = luaL_optstring(L, 2, "=(load)");
        // The reader's pieces, kept in a table while it runs, then joined
        lua_newtable(L);
        int pieces = lua_gettop(L);
        int n = 0;
        for (;;)
        {
            lua_pushvalue(L, 1);
            lua_call(L, 0, 1);
            if (lua_isnil(L, -1) || (lua_isstring(L, -1) && (lua_objlen(L, -1) == 0)))
            {
                lua_pop(L, 1);
                break;
            }
            if (lua_type(L, -1) != LUA_TSTRING)
                return luaL_error(L, "reader function must return a string");
            lua_rawseti(L, pieces, ++n);
        }
        luaL_Buffer b;
        luaL_buffinit(L, &b);
        for (int i = 1; i <= n; i++)
        {
            lua_rawgeti(L, pieces, i);
            luaL_addvalue(&b);
        }
        luaL_pushresult(&b);
        size_t len;
        const char *s = lua_tolstring(L, -1, &len);
        status = luaL_loadbufferx(L, s, len, chunkname, "t");
    }
    if (status == 0)
        sb_set_env(L, 4);
    return sb_load_result(L, status);
}

/** loadfile([filename [, mode [, env]]]) */
static int sb_loadfile(lua_State *L)
{
    const char *fname = luaL_optstring(L, 1, NULL);
    if (!sb_mode_allows_text(L, 2))
    {
        lua_pushnil(L);
        lua_pushstring(L, sb_binary_refused);
        return 2;
    }
    int status = luaL_loadfilex(L, fname, "t");
    if (status == 0)
        sb_set_env(L, 3);
    return sb_load_result(L, status);
}

/** dofile([filename]) */
static int sb_dofile(lua_State *L)
{
    const char *fname = luaL_optstring(L, 1, NULL);
    int base = lua_gettop(L);
    if (luaL_loadfilex(L, fname, "t") != 0)
        return lua_error(L);
    lua_call(L, 0, LUA_MULTRET);
    return lua_gettop(L) - base;
}

/** The library table on top of the stack, created on first use. */
static void sb_push_library(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, SB_LIBRARY_KEY);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, SB_LIBRARY_KEY);
    }
}

/** The loader require() gets for a game library file asked for under another name: the module
 *  already loaded under the name in upvalue 1. */
static int sb_library_alias(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, -1, lua_tostring(L, lua_upvalueindex(1)));
    return 1;
}

int lua_sandbox_load_module(lua_State *L, const char *name, const char *fname)
{
    // A module the game's own library loaded, asked for under another name once the debug library
    // is cut down (Heroes Journey: require "fxdata.lua.classes.Pos3d" for classes.Pos3d), would run
    // a second time without the debug functions it uses while loading. The one already loaded is
    // what such a script means; it gets that one.
    char real[PATH_MAX];
    const char *key = (realpath(fname, real) != NULL) ? real : fname;
    sb_push_library(L);
    int lib = lua_gettop(L);
    lua_getfield(L, LUA_REGISTRYINDEX, SB_CLOSED_KEY);
    int closed = lua_toboolean(L, -1);
    lua_pop(L, 1);
    if (closed)
    {
        lua_getfield(L, lib, key);
        if ((lua_type(L, -1) == LUA_TSTRING) && (strcmp(lua_tostring(L, -1), name) != 0))
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
            lua_getfield(L, -1, lua_tostring(L, -2));
            int loaded = !lua_isnil(L, -1);
            lua_pop(L, 2);
            if (loaded)
            {
                JUSTLOG("Lua: module '%s' is the game's own '%s'; using the one already loaded", name, lua_tostring(L, -1));
                lua_pushcclosure(L, sb_library_alias, 1);
                lua_remove(L, lib);
                return 0;
            }
        }
        lua_pop(L, 1);
    }
    int status = luaL_loadfilex(L, fname, "t");
    if ((status == 0) && !closed)
    {
        lua_pushstring(L, name);
        lua_setfield(L, lib, key);
    }
    lua_remove(L, lib);
    return status;
}

/** require()'s Lua-file loader, package.loaders[2], as LuaJIT's but text only: each
 *  package.path template with "?" replaced by the module name, dots turned into slashes. */
static int sb_loader_lua(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    const char *path = lua_tostring(L, -1);
    if (path == NULL)
        return luaL_error(L, "package.path must be a string");
    const char *modname = luaL_gsub(L, name, ".", "/");
    luaL_Buffer tried;
    luaL_buffinit(L, &tried);
    while (*path != '\0')
    {
        const char *sep = strchr(path, ';');
        size_t len = (sep != NULL) ? (size_t)(sep - path) : strlen(path);
        if (len > 0)
        {
            lua_pushlstring(L, path, len);
            const char *fname = luaL_gsub(L, lua_tostring(L, -1), "?", modname);
            lua_remove(L, -2);
            FILE *f = fopen(fname, "r");
            if (f != NULL)
            {
                fclose(f);
                if (lua_sandbox_load_module(L, name, fname) != 0)
                    return luaL_error(L, "error loading module '%s' from file '%s':\n\t%s",
                        name, fname, lua_tostring(L, -1));
                return 1;
            }
            lua_pushfstring(L, "\n\tno file '%s'", fname);
            lua_remove(L, -2);
            luaL_addvalue(&tried);
        }
        if (sep == NULL)
            break;
        path = sep + 1;
    }
    luaL_pushresult(&tried);
    return 1;
}

static void sb_set_global(lua_State *L, const char *name, lua_CFunction fn)
{
    lua_pushcfunction(L, fn);
    lua_setglobal(L, name);
}

/** LuaJIT loads its ffi library on demand when it parses a 64-bit integer or imaginary literal.
 *  Load it now and take the module away; its literal support stays. */
static void sb_drop_ffi(lua_State *L)
{
    static const char literal[] = "return 1LL";
    if (luaL_loadbufferx(L, literal, sizeof(literal) - 1, "=sandbox", "t") != 0)
    {
        lua_pop(L, 1); // built without FFI: there is nothing to take away
        return;
    }
    if (lua_pcall(L, 0, 0, 0) != 0)
        lua_pop(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    if (lua_istable(L, -1))
    {
        lua_pushnil(L);
        lua_setfield(L, -2, "ffi");
    }
    lua_pop(L, 1);
    lua_getglobal(L, "package");
    if (lua_istable(L, -1))
    {
        lua_getfield(L, -1, "preload");
        if (lua_istable(L, -1))
        {
            lua_pushnil(L);
            lua_setfield(L, -2, "ffi");
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_gc(L, LUA_GCCOLLECT, 0);
}

void lua_sandbox_open(lua_State *L)
{
    sb_drop_ffi(L);

    sb_set_global(L, "load", sb_load);
    sb_set_global(L, "loadstring", sb_loadstring);
    sb_set_global(L, "loadfile", sb_loadfile);
    sb_set_global(L, "dofile", sb_dofile);

    lua_getglobal(L, "package");
    if (lua_istable(L, -1))
    {
        lua_getfield(L, -1, "loaders");
        if (lua_istable(L, -1) && (lua_objlen(L, -1) >= 2))
        {
            lua_pushcfunction(L, sb_loader_lua);
            lua_rawseti(L, -2, 2);
        } else
        {
            WARNLOG("Lua: unexpected package.loaders layout; require() may still load bytecode");
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

void lua_sandbox_close_debug(lua_State *L)
{
    static const char *const keep[] = {"traceback", "getinfo", NULL};
    lua_newtable(L);
    lua_getglobal(L, "debug");
    if (lua_istable(L, -1))
    {
        for (const char *const *k = keep; *k != NULL; k++)
        {
            lua_getfield(L, -1, *k);
            lua_setfield(L, -3, *k);
        }
    }
    lua_pop(L, 1);
    lua_pushvalue(L, -1);
    lua_setglobal(L, "debug");
    lua_pushboolean(L, 1);
    lua_setfield(L, LUA_REGISTRYINDEX, SB_CLOSED_KEY);
    // require "debug" reads package.loaded, the registry's _LOADED
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    if (lua_istable(L, -1))
    {
        lua_pushvalue(L, -2);
        lua_setfield(L, -2, "debug");
    }
    lua_pop(L, 2);
}

int lua_sandbox_loadfile(lua_State *L, const char *fname)
{
    return luaL_loadfilex(L, fname, "t");
}

int lua_sandbox_dostring(lua_State *L, const char *code)
{
    int status = luaL_loadbufferx(L, code, strlen(code), code, "t");
    if (status == 0)
        status = lua_pcall(L, 0, LUA_MULTRET, 0);
    return status;
}
/******************************************************************************/
