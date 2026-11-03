/******************************************************************************/
/** @file lua_readonly_io.c
 *     Read-only file access for level and campaign Lua scripts (this edition).
 * @par Purpose:
 *     Level and campaign scripts arrive with downloaded maps and Workshop items.
 *     Upstream's sandbox (lua_base.c, #5389) removes most of the io and os
 *     libraries but keeps io.open and os.remove "for now", so one line in a
 *     map's script could overwrite or delete any file the player owns
 *     (~/.bashrc, ~/.config/autostart, the saves). Nothing the game ships, nor
 *     anything in upstream's tree, writes or deletes files from Lua; only the
 *     bundled binser library has file helpers, and nothing calls them.
 *
 *     So scripts keep reading files and lose the rest:
 *     - io.open opens for reading only; a write, append or update mode returns
 *       nil plus a message, as a failed open always has, so a script that
 *       checks the result carries on;
 *     - os.remove returns nil plus a message the same way;
 *     - require() no longer loads native libraries (.so): a map pack could
 *       ship one beside its scripts, and loading it runs native code.
 *
 *     io.open is a new implementation, not a wrapper around the original. The
 *     game's own scripts need the debug library (serialisation and the trigger
 *     system use getregistry, getupvalue and getinfo), and the debug library
 *     can reach anything a wrapper keeps -- an upvalue, a registry entry, a
 *     frame seen from a hook. With nothing kept, there is nothing to reach: the
 *     original io.open becomes garbage. For the same reason a file handle
 *     holds no pointer: it is a slot number and a generation, checked against
 *     the table below, so a userdata dressed up as a handle (debug.setmetatable)
 *     can at worst name another read-only file.
 *
 *     On its own this closed the one-line routes only: bytecode and LuaJIT's FFI
 *     reached further. lua_sandbox.c closes those, and cuts the debug library
 *     down for scripts loaded after the game's own library.
 *
 *     The read functions follow Lua 5.1's liolib.c (MIT licence).
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
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "lua_readonly_io.h"
#include "bflib_basics.h"
#include "bflib_fileio.h"
#include "globals.h"

#include "post_inc.h"

#define RO_FILE_METATABLE "KeeperFX.ReadOnlyFile"
#define RO_MAX_FILES 32
#define RO_MAX_LINES_FORMATS 8
#define RO_MAX_LOGGED_REFUSALS 10

struct RoSlot {
    FILE *fp;
    uint32_t gen;
};

/** What a script's file handle holds: never a pointer. */
struct RoHandle {
    int32_t slot;
    uint32_t gen;
};

static struct RoSlot ro_slots[RO_MAX_FILES];
static int ro_refusals_logged;

/******************************************************************************/

/** The open file behind argument idx, or NULL if it has been closed. */
static FILE *ro_check_file(lua_State *L, int idx)
{
    const struct RoHandle *h = (const struct RoHandle *)luaL_checkudata(L, idx, RO_FILE_METATABLE);
    if (lua_objlen(L, idx) != sizeof(struct RoHandle))
        luaL_argerror(L, idx, "not a file");
    if ((h->slot < 0) || (h->slot >= RO_MAX_FILES))
        return NULL;
    const struct RoSlot *s = &ro_slots[h->slot];
    if ((s->fp == NULL) || (s->gen != h->gen))
        return NULL;
    return s->fp;
}

static FILE *ro_open_file(lua_State *L, int idx)
{
    FILE *fp = ro_check_file(L, idx);
    if (fp == NULL)
        luaL_error(L, "attempt to use a closed file");
    return fp;
}

static void ro_close_slot(struct RoHandle *h)
{
    if ((h->slot < 0) || (h->slot >= RO_MAX_FILES))
        return;
    struct RoSlot *s = &ro_slots[h->slot];
    if ((s->fp != NULL) && (s->gen == h->gen))
    {
        fclose(s->fp);
        s->fp = NULL;
        s->gen++;
    }
    h->slot = -1;
}

static int ro_push_error(lua_State *L, const char *what, int en)
{
    lua_pushnil(L);
    lua_pushfstring(L, "%s: %s", what, strerror(en));
    lua_pushinteger(L, en);
    return 3;
}

static int ro_refuse(lua_State *L, const char *func, const char *fname, const char *detail)
{
    if (ro_refusals_logged < RO_MAX_LOGGED_REFUSALS)
    {
        ro_refusals_logged++;
        WARNLOG("Lua: refused %s(\"%s\"%s): scripts may read files, not change them", func, fname, detail);
    }
    lua_pushnil(L);
    lua_pushfstring(L, "%s: scripts may read files, not change them", fname);
    lua_pushinteger(L, EACCES);
    return 3;
}

/******************************************************************************/
/* Reading, as Lua 5.1's liolib.c does it. Each reader pushes one value and
 * returns whether it read anything. */

static int ro_test_eof(lua_State *L, FILE *f)
{
    int c = getc(f);
    ungetc(c, f);
    lua_pushlstring(L, NULL, 0);
    return (c != EOF);
}

static int ro_read_number(lua_State *L, FILE *f)
{
    lua_Number d;
    if (fscanf(f, LUA_NUMBER_SCAN, &d) == 1)
    {
        lua_pushnumber(L, d);
        return 1;
    }
    lua_pushnil(L);
    return 0;
}

static int ro_read_line(lua_State *L, FILE *f, int keep_eol)
{
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    for (;;)
    {
        char *p = luaL_prepbuffer(&b);
        if (fgets(p, LUAL_BUFFERSIZE, f) == NULL)
        {
            luaL_pushresult(&b);
            return (lua_objlen(L, -1) > 0);
        }
        size_t l = strlen(p);
        if ((l == 0) || (p[l-1] != '\n'))
        {
            luaL_addsize(&b, l);
        } else
        {
            luaL_addsize(&b, keep_eol ? l : l - 1);
            luaL_pushresult(&b);
            return 1;
        }
    }
}

static int ro_read_chars(lua_State *L, FILE *f, size_t n)
{
    size_t rlen = LUAL_BUFFERSIZE;
    size_t nr;
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    do {
        char *p = luaL_prepbuffer(&b);
        if (rlen > n)
            rlen = n;
        nr = fread(p, 1, rlen, f);
        luaL_addsize(&b, nr);
        n -= nr;
    } while ((n > 0) && (nr == rlen));
    luaL_pushresult(&b);
    return (n == 0) || (lua_objlen(L, -1) > 0);
}

/** Read with the formats at stack positions first..top. */
static int ro_g_read(lua_State *L, FILE *f, int first)
{
    int nargs = lua_gettop(L) - first + 1;
    int success;
    int n;
    clearerr(f);
    if (nargs <= 0)
    {
        success = ro_read_line(L, f, 0);
        n = first + 1;
    } else
    {
        luaL_checkstack(L, nargs + LUA_MINSTACK, "too many arguments");
        success = 1;
        for (n = first; (nargs-- > 0) && success; n++)
        {
            if (lua_type(L, n) == LUA_TNUMBER)
            {
                lua_Integer k = lua_tointeger(L, n);
                luaL_argcheck(L, k >= 0, n, "negative count");
                success = (k == 0) ? ro_test_eof(L, f) : ro_read_chars(L, f, (size_t)k);
                continue;
            }
            const char *p = luaL_checkstring(L, n);
            if (*p == '*')
                p++;  // "*l" as in Lua 5.1, or "l" as in 5.2 and LuaJIT
            switch (*p)
            {
            case 'n':
                success = ro_read_number(L, f);
                break;
            case 'l':
                success = ro_read_line(L, f, 0);
                break;
            case 'L':
                success = ro_read_line(L, f, 1);
                break;
            case 'a':
                ro_read_chars(L, f, ~((size_t)0));
                success = 1;
                break;
            default:
                return luaL_argerror(L, n, "invalid format");
            }
        }
    }
    if (ferror(f))
        return ro_push_error(L, "read", errno);
    if (!success)
    {
        lua_pop(L, 1);
        lua_pushnil(L);
    }
    return n - first;
}

/******************************************************************************/
/* File methods */

static int ro_f_read(lua_State *L)
{
    return ro_g_read(L, ro_open_file(L, 1), 2);
}

static int ro_lines_iterator(lua_State *L)
{
    int nformats = (int)lua_tointeger(L, lua_upvalueindex(2));
    lua_settop(L, 0);
    lua_pushvalue(L, lua_upvalueindex(1));
    FILE *f = ro_check_file(L, 1);
    if (f == NULL)
        return luaL_error(L, "file is already closed");
    for (int i = 0; i < nformats; i++)
        lua_pushvalue(L, lua_upvalueindex(3 + i));
    return ro_g_read(L, f, 2);
}

static int ro_f_lines(lua_State *L)
{
    ro_open_file(L, 1);
    int nformats = lua_gettop(L) - 1;
    luaL_argcheck(L, nformats <= RO_MAX_LINES_FORMATS, RO_MAX_LINES_FORMATS + 2, "too many formats");
    lua_pushvalue(L, 1);
    lua_pushinteger(L, nformats);
    for (int i = 0; i < nformats; i++)
        lua_pushvalue(L, 2 + i);
    lua_pushcclosure(L, ro_lines_iterator, 2 + nformats);
    return 1;
}

static int ro_f_seek(lua_State *L)
{
    static const int mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
    static const char *const modenames[] = {"set", "cur", "end", NULL};
    FILE *f = ro_open_file(L, 1);
    int op = luaL_checkoption(L, 2, "cur", modenames);
    lua_Number offset = luaL_optnumber(L, 3, 0);
    if (fseeko(f, (off_t)offset, mode[op]) != 0)
        return ro_push_error(L, "seek", errno);
    lua_pushnumber(L, (lua_Number)ftello(f));
    return 1;
}

static int ro_f_close(lua_State *L)
{
    ro_open_file(L, 1);
    ro_close_slot((struct RoHandle *)lua_touserdata(L, 1));
    lua_pushboolean(L, 1);
    return 1;
}

static int ro_f_gc(lua_State *L)
{
    // Only ever reached for a real handle: the metatable is ours, and a forged
    // one (wrong size) is left alone rather than reinterpreted.
    if (lua_objlen(L, 1) == sizeof(struct RoHandle))
        ro_close_slot((struct RoHandle *)lua_touserdata(L, 1));
    return 0;
}

static int ro_f_tostring(lua_State *L)
{
    if (ro_check_file(L, 1) == NULL)
        lua_pushliteral(L, "file (closed)");
    else
        lua_pushfstring(L, "file (read-only, %p)", lua_touserdata(L, 1));
    return 1;
}

/******************************************************************************/
/* The replacements */

/** Modes that only read: "r", optionally followed by "b" or "t". */
static int ro_mode_is_read_only(const char *mode)
{
    if (mode[0] != 'r')
        return 0;
    for (const char *p = mode + 1; *p != '\0'; p++)
    {
        if ((*p != 'b') && (*p != 't'))
            return 0;
    }
    return 1;
}

static int ro_io_open(lua_State *L)
{
    const char *fname = luaL_checkstring(L, 1);
    const char *mode = luaL_optstring(L, 2, "r");
    if (!ro_mode_is_read_only(mode))
    {
        char detail[64];
        snprintf(detail, sizeof(detail), ", \"%s\"", mode);
        return ro_refuse(L, "io.open", fname, detail);
    }
    // The handle exists before the file is opened, so an allocation failure
    // (which unwinds) cannot leak an open FILE.
    struct RoHandle *h = (struct RoHandle *)lua_newuserdata(L, sizeof(struct RoHandle));
    h->slot = -1;
    h->gen = 0;
    luaL_getmetatable(L, RO_FILE_METATABLE);
    lua_setmetatable(L, -2);

    // Scripts written on Windows name files in any case, as the rest of the
    // game's Lua file handling already allows for.
    char disk_fname[PATH_MAX];
    FILE *fp = fopen(LbFileCaseInsensitivePath(fname, disk_fname, sizeof(disk_fname)), "rb");
    if (fp == NULL)
        return ro_push_error(L, fname, errno);
    // Pick the slot only now, with no Lua call left before it is taken: the
    // allocation above can run a garbage-collection step, and a script's __gc
    // finalizer could open a file itself and take a slot chosen earlier.
    int slot = -1;
    for (int i = 0; i < RO_MAX_FILES; i++)
    {
        if (ro_slots[i].fp == NULL)
        {
            slot = i;
            break;
        }
    }
    if (slot < 0)
    {
        fclose(fp);
        return ro_push_error(L, fname, EMFILE);
    }
    ro_slots[slot].fp = fp;
    h->slot = slot;
    h->gen = ro_slots[slot].gen;
    return 1;
}

static int ro_os_remove(lua_State *L)
{
    return ro_refuse(L, "os.remove", luaL_checkstring(L, 1), "");
}

/******************************************************************************/

static void ro_set_library_function(lua_State *L, const char *lib, const char *name, lua_CFunction fn)
{
    lua_getglobal(L, lib);
    if (lua_istable(L, -1))
    {
        lua_pushcfunction(L, fn);
        lua_setfield(L, -2, name);
    }
    lua_pop(L, 1);
}

/** require() searches package.loaders in order: [1] preload, [2] Lua files,
 *  [3] native libraries on package.cpath, [4] the native "root" loader. The
 *  last two are dropped, not just given an empty cpath, since a script can set
 *  package.cpath itself. Called before setLuaPath() appends this edition's
 *  case-insensitive searcher. */
static void ro_drop_native_loaders(lua_State *L)
{
    lua_getglobal(L, "package");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return;
    }
    lua_getfield(L, -1, "loaders");
    if (lua_istable(L, -1) && (lua_objlen(L, -1) == 4))
    {
        lua_pushnil(L);
        lua_rawseti(L, -2, 4);
        lua_pushnil(L);
        lua_rawseti(L, -2, 3);
    } else
    {
        WARNLOG("Lua: unexpected package.loaders layout; native modules not disabled");
    }
    lua_pop(L, 1);
    lua_pushliteral(L, "");
    lua_setfield(L, -2, "cpath");
    lua_pop(L, 1);
}

void lua_make_files_read_only(lua_State *L)
{
    ro_refusals_logged = 0;

    luaL_newmetatable(L, RO_FILE_METATABLE);
    static const luaL_Reg methods[] = {
        {"read",  ro_f_read},
        {"lines", ro_f_lines},
        {"seek",  ro_f_seek},
        {"close", ro_f_close},
        {NULL, NULL}
    };
    lua_newtable(L);
    luaL_register(L, NULL, methods);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, ro_f_gc);
    lua_setfield(L, -2, "__gc");
    lua_pushcfunction(L, ro_f_tostring);
    lua_setfield(L, -2, "__tostring");
    lua_pop(L, 1);

    ro_set_library_function(L, "io", "open", ro_io_open);
    ro_set_library_function(L, "os", "remove", ro_os_remove);
    ro_drop_native_loaders(L);
}
