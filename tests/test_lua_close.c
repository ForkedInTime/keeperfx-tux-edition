/* lua_close() after the sandbox has taken the ffi module away (issue #91).
 *
 * On LuaJIT before its commit f5affaa6c4 (2024-04-18, "Turn FFI finalizer table into a proper GC
 * root") the table of cdata finalizers is kept alive only by the ffi module: it is the environment of
 * ffi.gc. lua_close() walks that table. lua_sandbox.c loads the module once, drops it and collects,
 * and on those LuaJIT versions the collection freed the table: every lua_close(), one per level change,
 * read and wrote freed memory, and a game crashed in lua_close() when that memory had been reused or
 * returned to the system. The AppImage carries Ubuntu 24.04's LuaJIT of 2023-12-23.
 *
 * Here the state's memory comes from an allocator that poisons what is freed and never hands it out
 * again, so reading freed memory faults every time instead of now and then. Each case runs in a child
 * process; a crash is a failure, not the end of the run. On a LuaJIT with the fix the cases pass with
 * or without this edition's change; run them with LD_LIBRARY_PATH set to an older libluajit-5.1.so.2
 * (tests/run.sh does when KFX_TEST_OLD_LUAJIT_DIR names its folder) to see what they guard. */
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lua_sandbox.h"

/* What lua_sandbox.c reaches of the engine. */
int LbErrorLog(const char *format, ...) { (void)format; return 0; }
int LbWarnLog(const char *format, ...) { (void)format; return 0; }
int LbSyncLog(const char *format, ...) { (void)format; return 0; }
int LbJustLog(const char *format, ...) { (void)format; return 0; }
unsigned int get_gameturn(void) { return 0; }

static int g_failures = 0;

static void expect(int ok, const char *what)
{
    printf(ok ? "  ok   %s\n" : "  FAIL %s\n", what);
    if (!ok)
        g_failures++;
}

static void *poison_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    (void)ud;
    if (nsize == 0)
    {
        if (ptr != NULL)
            memset(ptr, 0xA5, osize); /* poisoned, and never handed out again */
        return NULL;
    }
    void *block = malloc(nsize);
    if ((block != NULL) && (ptr != NULL))
    {
        memcpy(block, ptr, (osize < nsize) ? osize : nsize);
        memset(ptr, 0xA5, osize);
    }
    return block;
}

/* As open_lua_script() builds the state, without the game's own scripts. */
static lua_State *sandboxed_state(void)
{
    lua_State *L = lua_newstate(poison_alloc, NULL);
    if (L == NULL)
        return NULL;
    static const luaL_Reg stdlib[] = {
        {"",               luaopen_base},
        {LUA_TABLIBNAME,   luaopen_table},
        {LUA_IOLIBNAME,    luaopen_io},
        {LUA_OSLIBNAME,    luaopen_os},
        {LUA_STRLIBNAME,   luaopen_string},
        {LUA_MATHLIBNAME,  luaopen_math},
        {LUA_DBLIBNAME,    luaopen_debug},
        {LUA_LOADLIBNAME,  luaopen_package},
        {LUA_BITLIBNAME,   luaopen_bit},
        {NULL, NULL}
    };
    for (const luaL_Reg *lib = stdlib; lib->func != NULL; lib++)
    {
        lua_pushcfunction(L, lib->func);
        lua_pushstring(L, lib->name);
        lua_call(L, 1, 0);
    }
    lua_sandbox_open(L);
    return L;
}

/* Runs a chunk that must return true; returns whether it did. */
static int chunk_true(lua_State *L, const char *chunk)
{
    if (luaL_loadstring(L, chunk) != 0 || lua_pcall(L, 0, 1, 0) != 0)
    {
        printf("       %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return 0;
    }
    int ok = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return ok;
}

/* A level: the sandbox, a script that makes garbage and 64-bit numbers, then lua_close(). */
static int play_a_level(void)
{
    lua_State *L = sandboxed_state();
    if (L == NULL)
        return 2;
    if (!chunk_true(L, "local t = {} for i = 1, 20000 do t[i] = {i, tostring(i) .. 'x'} end "
                       "local n = 1LL for i = 1, 30 do n = n * 3 + 1 end t = nil collectgarbage() "
                       "return n > 0"))
        return 3;
    lua_gc(L, LUA_GCCOLLECT, 0);
    lua_close(L);
    return 0;
}

static int case_one_level(void) { return play_a_level(); }

static int case_ten_levels(void)
{
    for (int i = 0; i < 10; i++)
    {
        int r = play_a_level();
        if (r != 0)
            return r;
    }
    return 0;
}

static int case_still_no_ffi(void)
{
    lua_State *L = sandboxed_state();
    if (L == NULL)
        return 2;
    int ok = chunk_true(L, "return package.loaded.ffi == nil and not pcall(require, 'ffi') and "
                           "tostring(1LL + 1) == '2LL'");
    lua_close(L);
    return ok ? 0 : 3;
}

/* The FFI's metamethod table (miscmap): any cdata metamethod a script triggers reads it -- an unknown
 * field, arithmetic with a table, a call, a concatenation, a length, a comparison. Each must be a clean
 * Lua error, then the state closes. Before it was kept alive the drop freed it, on every LuaJIT. */
static int case_cdata_metamethods(void)
{
    lua_State *L = sandboxed_state();
    if (L == NULL)
        return 2;
    int ok = chunk_true(L, "local x = 1LL "
                           "for _, f in ipairs({function() return x.foo end, function() return x + {} end, "
                           "function() return x() end, function() return x .. 1 end, function() return #x end, "
                           "function() return x < {} end, function() return pairs(x) end}) do "
                           "if pcall(f) then return false end end "
                           "collectgarbage() return true");
    lua_gc(L, LUA_GCCOLLECT, 0);
    lua_close(L);
    return ok ? 0 : 3;
}

/* Runs a case in a child, so a crash is reported instead of ending the test. */
static void run_case(const char *what, int (*body)(void))
{
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0)
    {
        expect(0, "could not start a test process");
        return;
    }
    if (pid == 0)
    {
        int result = body();
        fflush(stdout);
        _exit(result);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status))
    {
        char line[200];
        snprintf(line, sizeof(line), "%s (crashed: signal %d)", what, WTERMSIG(status));
        expect(0, line);
        return;
    }
    expect(WIFEXITED(status) && (WEXITSTATUS(status) == 0), what);
}

int main(void)
{
    printf("lua_close after the sandbox\n");
    run_case("a level's state closes after the sandbox took the ffi module away", case_one_level);
    run_case("ten levels in a row close", case_ten_levels);
    run_case("scripts still can't reach the ffi module, and 64-bit literals still work", case_still_no_ffi);
    run_case("cdata metamethods a script triggers are clean errors, and the state closes", case_cdata_metamethods);
    if (g_failures > 0)
    {
        printf("%d failure(s)\n", g_failures);
        return 1;
    }
    printf("all lua_close tests passed\n");
    return 0;
}
