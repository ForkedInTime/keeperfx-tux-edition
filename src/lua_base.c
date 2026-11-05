#include "pre_inc.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <limits.h>
#include <string.h>

#include "lua_api.h"
#include "lua_readonly_io.h"
#include "lua_sandbox.h"

#include "bflib_basics.h"
#include "bflib_fileio.h"
#include "config_keeperfx.h"
#include "lens_api.h"
#include "game_legacy.h"
#include "globals.h"
#include "gui_msgs.h"
#include "replay.h"

#include "config_creature.h"
#include "config_terrain.h"
#include "config_magic.h"
#include "config_effects.h"
#include "config_objects.h"
#include "config_trapdoor.h"
#include "config_crtrstates.h"

#include "post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct lua_State *Lvl_script = NULL;

/* Lua opens script files itself, with a plain fopen() of the name as spelt.
 * Everything below decides whether a script exists with LbFileExists(), which
 * matches case-insensitively (as LbFileOpen does), so a campaign whose
 * LEVELS_LOCATION says campgns/MyCamp while the folder on disk is
 * campgns/mycamp passes the check and then its map's .lua silently fails to
 * load. Windows' filesystem matches case itself, so the same campaign works
 * there. Hand Lua the name that is really on disk instead -- for every
 * luaL_dofile() in this file, so script loading added upstream later is
 * covered without touching each call. */
static int lua_dofile_on_disk(lua_State *L, const char *fname)
{
    char disk_fname[PATH_MAX];
    const char *path = LbFileCaseInsensitivePath(fname, disk_fname, sizeof(disk_fname));
    // Text only, as every load in this file (lua_sandbox.c)
    return lua_sandbox_loadfile(L, path) || lua_pcall(L, 0, LUA_MULTRET, 0);
}
#undef luaL_dofile
#define luaL_dofile(L, fname) lua_dofile_on_disk(L, fname)

/* The same mismatch for require(): Lua's own searcher fopen()s each
 * package.path candidate exactly as spelt, so a module under that campaign's
 * folder -- or require "Utils" beside a real utils.lua -- is not found. This
 * searcher runs after Lua's own ones, so it only ever sees names they could
 * not find, and resolves each candidate the way LbFileOpen does. */
static int lua_searcher_on_disk(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    const char *path = lua_tostring(L, -1);
    if (path == NULL)
        return 0;
    const char *modname = luaL_gsub(L, name, ".", "/");
    while (*path != '\0')
    {
        const char *sep = strchr(path, ';');
        size_t len = (sep != NULL) ? (size_t)(sep - path) : strlen(path);
        if (len > 0)
        {
            lua_pushlstring(L, path, len);
            const char *candidate = luaL_gsub(L, lua_tostring(L, -1), "?", modname);
            if (LbFileExists(candidate))
            {
                char disk_fname[PATH_MAX];
                LbFileCaseInsensitivePath(candidate, disk_fname, sizeof(disk_fname));
                if (lua_sandbox_load_module(L, name, disk_fname) != LUA_OK)
                    return luaL_error(L, "error loading module '%s' from file '%s':\n\t%s", name, disk_fname, lua_tostring(L, -1));
                return 1;
            }
            lua_pop(L, 2);
        }
        if (sep == NULL)
            break;
        path = sep + 1;
    }
    return 0;
}

static void add_lua_searcher_on_disk(lua_State *L)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "searchers"); // Lua 5.2+
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        lua_getfield(L, -1, "loaders"); // Lua 5.1 / LuaJIT
    }
    if (lua_istable(L, -1))
    {
#if LUA_VERSION_NUM >= 502
        int count = (int)lua_rawlen(L, -1);
#else
        int count = (int)lua_objlen(L, -1);
#endif
        lua_pushcfunction(L, lua_searcher_on_disk);
        lua_rawseti(L, -2, count + 1);
    }
    lua_pop(L, 2);
}


static int lua_disabled_os_function(lua_State *L)
{
    const char *func_name = lua_tostring(L, lua_upvalueindex(1));
    WARNLOG("os.%s is disabled because it is not synchronized in multiplayer; use GetGameTimestamp", func_name);
    return luaL_error(L, "os.%s is disabled because it is not synchronized in multiplayer; use GetGameTimestamp", func_name);
}


static void lua_blacklist(lua_State *L, const char *libname, const char *name)
{
    lua_getglobal(L, libname);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    lua_pushnil(L);
    lua_setfield(L, -2, name);
    lua_pop(L, 1);
}

// Removes every function of a library but the ones named in keep, a list ending with NULL
static void lua_whitelist(lua_State *L, const char *libname, const char *const keep[])
{
    lua_getglobal(L, libname);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    int t = lua_gettop(L);
    lua_pushnil(L);
    while (lua_next(L, t) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING) {
            const char *key = lua_tostring(L, -2);
            const char *const *k = keep;
            while ((*k != NULL) && (strcmp(key, *k) != 0))
                k++;
            if (*k == NULL) {
                lua_pushnil(L);
                lua_setfield(L, t, key);
            }
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

// Remove Lua functions which can break the sandbox/determinism
static void disable_lua_functions(lua_State *L)
{
    // TODO - remove/replace these
    lua_whitelist(L, "io", (const char *const[]){"open", NULL});

    // TODO - remove/replace these
    // This edition also keeps the clock functions: they read the time and reach nothing outside
    // the game, and scripts use them, as the Pandaemonium campaign's second level does for its
    // welcome message (without os.date its whole setup stopped there). Multiplayer replaces
    // time, date and clock below, since they are not synchronised.
    lua_whitelist(L, "os", (const char *const[]){"remove", "date", "time", "clock", "difftime", NULL});

    lua_blacklist(L, "package", "loadlib");

    if (game.game_kind != GKind_MultiGame) {
        return;
    }
    const char *disabled_os_functions[] = {"time", "date", "clock"};

    lua_getglobal(L, "os");
    if (lua_istable(L, -1)) {
        for (int i = 0; i < sizeof(disabled_os_functions) / sizeof(disabled_os_functions[0]); i++) {
            lua_pushstring(L, disabled_os_functions[i]);
            lua_pushcclosure(L, lua_disabled_os_function, 1);
            lua_setfield(L, -2, disabled_os_functions[i]);
        }
    }
    lua_pop(L, 1);
}


TbBool CheckLua(lua_State *L, int result, const char* func)
{
    if (result != LUA_OK) {
        int error_type = lua_type(L, -1);
        const char *message = lua_typename(L, error_type);
        if (error_type == LUA_TSTRING) {
            message = lua_tostring(L, -1);
        }
        ERRORLOG("Lua error in %s: %s", func, message);
        lua_pop(L, 1); // pop error object

        if (exit_on_lua_error) {
            ERRORLOG("Exiting due to Lua error");
            exit(EXIT_FAILURE);
        }
        return false;
    }
    return true;
}


void close_lua_script()
{
    LensManager_ClearCustomLenses(LensManager_GetInstance());
    if(Lvl_script)
        lua_close(Lvl_script);
    Lvl_script = NULL;
}


/* This edition: the path the game's own library (fxdata/lua/init.lua) loads with: fxdata alone,
 * so that no campaign or level file of the same name stands in for one of its modules while the
 * full debug library is still there (lua_sandbox.c). The default path it replaces is kept in
 * default_path for setLuaPath() to build on once the library has loaded. */
static void setLuaLibraryPath(lua_State *L, char *default_path, size_t len)
{
    char fx_data_path[4096];
    prepare_file_path_buf(fx_data_path, sizeof(fx_data_path), FGrp_FxData, "lua/?.lua");
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    snprintf(default_path, len, "%s", lua_tostring(L, -1) ? lua_tostring(L, -1) : "");
    lua_pop(L, 1);
    lua_pushstring(L, fx_data_path);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
}

static void restoreLuaDefaultPath(lua_State *L, const char *default_path)
{
    lua_getglobal(L, "package");
    lua_pushstring(L, default_path);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
}

int setLuaPath( lua_State* L)
{
    #define PATH_LENGTH 4096
    char fx_data_path[PATH_LENGTH];
    prepare_file_path_buf(fx_data_path, sizeof(fx_data_path), FGrp_FxData, "lua/?.lua");
    char campaignpath[PATH_LENGTH];
    prepare_file_path_buf(campaignpath, sizeof(campaignpath), FGrp_CmpgConfig, "lua/?.lua");
    char levelpath[PATH_LENGTH];
    prepare_file_path_buf(levelpath, sizeof(levelpath), FGrp_CmpgLvls, "?.lua");


    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path"); // get field "path" from table at top of stack (-1)
    const char *cur_path = lua_tostring(L, -1); // grab path string from top of stack
    char new_path[PATH_LENGTH];
    snprintf(new_path, sizeof(new_path), "%s;%s;%s;%s", levelpath,campaignpath,fx_data_path,cur_path);
    lua_pop(L, 1); // get rid of the string on the stack we just pushed on line 5
    lua_pushstring(L, new_path); // push the new one
    lua_setfield(L, -2, "path"); // set the field "path" in table at -2 with value at top of stack
    lua_pop(L, 1); // get rid of package table from top of stack
    add_lua_searcher_on_disk(L);
    return 0; // all done!
}


// Function to execute a piece of Lua code, and on failure, print the error message to the console
TbBool execute_lua_code_from_console(const char* code)
{
    if (Lvl_script == NULL) {
        ERRORLOG("Lua state is not initialized");
        return false;
    }

    if (code == NULL) {
        message_add(MsgType_Blank, 0, "no Lua code provided");
        return false;
    }

    // lua_sandbox_dostring, like luaL_dostring, leaves the error message on failure, or whatever the code returned on success.
    // Popping a fixed count took one slot too many after every error, and the sixth error ended the game.
    int top = lua_gettop(Lvl_script);
    int result = lua_sandbox_dostring(Lvl_script, code);
    if (result != LUA_OK) {
        const char *message = lua_tostring(Lvl_script, -1);
        ERRORLOG("Failed to execute Lua code: %s", message ? message : "Unknown error");

        // Pass the error message to message_add
        message_add(MsgType_Blank, 0, "lua error, see log");

        lua_settop(Lvl_script, top);
        return false;
    }

    lua_settop(Lvl_script, top);
    return true;
}

TbBool execute_lua_code_from_script(const char* code)
{
    if (Lvl_script == NULL) {
        ERRORLOG("Lua state is not initialized");
        return false;
    }

    if (code == NULL) {
        return false;
    }

    // Same stack handling as execute_lua_code_from_console()
    int top = lua_gettop(Lvl_script);
    int result = lua_sandbox_dostring(Lvl_script, code);
    if (result != LUA_OK) {
        const char *message = lua_tostring(Lvl_script, -1);
        ERRORLOG("Failed to execute Lua code: %s", message ? message : "Unknown error");

        lua_settop(Lvl_script, top);
        return false;
    }

    lua_settop(Lvl_script, top);
    return true;
}
void lua_set_random_seed(unsigned int seed)
{
    if (Lvl_script == NULL) {
        ERRORLOG("Lua state is not initialized");
        return;
    }

    lua_getglobal(Lvl_script, "math");
    if (lua_istable(Lvl_script, -1)) {
        lua_getfield(Lvl_script, -1, "randomseed");
        if (lua_isfunction(Lvl_script, -1)) {
            lua_pushinteger(Lvl_script, seed);
            if (!CheckLua(Lvl_script, lua_pcall(Lvl_script, 1, 0, 0), "math.randomseed")) {
                ERRORLOG("Failed to call math.randomseed");
            }
        } else {
            ERRORLOG("failed to find math.randomseed function");
            lua_pop(Lvl_script, 1); // Pop nil
        }
    } else {
        ERRORLOG("failed to find math table");
    }
    lua_pop(Lvl_script, 1); // Pop math table or nil
}


/* @comment
 *     The loading items of open_lua_script and open_lua_script_for_mod need to be consistent.
 * @note
 *     We do not setLuaPath for mod, so can load only a single lua file without dependencies.
 */
static TbBool open_lua_script_for_mod(lua_State* L, LevelNumber lvnum, const struct ModConfigItem *mod_item)
{
    TbBool result = false;
    const struct ModExistState *mod_state = &mod_item->state;
    char* fname = NULL;
    char mod_dir[256] = {0};
    sprintf(mod_dir, "%s/%s", MODS_DIR_NAME, mod_item->name);

    // If the mod name contains characters that the identifier does not support, ignore it.
    if (strpbrk(mod_item->name, " -") != NULL)
        return false;

    if (mod_state->fx_data)
    {
        fname = prepare_file_fmtpath_mod(mod_dir, FGrp_FxData, "lua/init.lua");
        if (LbFileExists(fname))
        {
            if (CheckLua(L, luaL_dofile(L, fname), "mod_fxdata_lua_file"))
            {
                result = true;
            }
            else
            {
                ERRORLOG("failed to load mod fxdata lua script for %s", mod_item->name);
            }
        }
    }

    if (mod_state->cmpg_config)
    {
        fname = prepare_file_fmtpath_mod(mod_dir, FGrp_CmpgConfig, "lua/init.lua");
        if (LbFileExists(fname))
        {
            if (CheckLua(L, luaL_dofile(L, fname), "mod_campaign_lua_file"))
            {
                result = true;
            }
            else
            {
                ERRORLOG("failed to load mod campaign lua script for %s", mod_item->name);
            }
        }
    }

    if (mod_state->cmpg_lvls)
    {
        short fgroup = get_level_fgroup(lvnum);
        fname = prepare_file_fmtpath_mod(mod_dir, fgroup, "map%05lu.lua", (unsigned long)lvnum);
        if (LbFileExists(fname))
        {
            if(CheckLua(L, luaL_dofile(L, fname), "mod_level_script_loading"))
            {
                result = true;
            }
            else
            {
                ERRORLOG("failed to load mod level lua script for %s", mod_item->name);
            }
        }
    }

    return result;
}

static void open_lua_script_for_mod_list(lua_State* L, LevelNumber lvnum, const struct ModConfigItem *mod_items, long mod_cnt)
{
    for (long i=0; i<mod_cnt; i++)
    {
        const struct ModConfigItem *mod_item = mod_items + i;
        if (open_lua_script_for_mod(L, lvnum, mod_item))
        {
            // If lua_on_start_for_mod_list has performance issues, a variable can be used to record the state here.
        }
    }
}

static void open_lua_script_for_mod_all(lua_State* L, LevelNumber lvnum)
{
    if (mods_conf.after_base_cnt > 0)
    {
        open_lua_script_for_mod_list(L, lvnum, mods_conf.after_base_item, mods_conf.after_base_cnt);
    }

    if (mods_conf.after_campaign_cnt > 0)
    {
        open_lua_script_for_mod_list(L, lvnum, mods_conf.after_campaign_item, mods_conf.after_campaign_cnt);
    }

    if (mods_conf.after_map_cnt > 0)
    {
        open_lua_script_for_mod_list(L, lvnum, mods_conf.after_map_item, mods_conf.after_map_cnt);
    }
}

// disable filesystem io during replays
// TODO: replay-correct save/load/logging API for Lua
static const char replay_file_io_stubs[] =
    "local function nop() end\n"
    "local f = {}\n"
    "function f:read() return nil end\n"
    "function f:write() return self end\n"
    "function f:lines() return nop end\n"
    "function f:seek() return 0 end\n"
    "function f:flush() return self end\n"
    "function f:setvbuf() return true end\n"
    "function f:close() return true end\n"
    "io.open = function(_, mode)\n"
    "    if mode == nil or mode:sub(1, 1) == 'r' and not mode:find('+', 1, true) then return nil, 'file access is disabled in replays' end\n"
    "    return f\n"
    "end\n"
    "io.lines = function() return nop end\n"
    "io.read = function() return nil end\n"
    "io.write = function() return f end\n"
    "io.input = function() return f end\n"
    "io.output = function() return f end\n"
    "io.close = function() return true end\n"
    "io.popen = function() return nil, 'file access is disabled in replays' end\n"
    "io.tmpfile = function() return f end\n"
    "os.remove = function() return true end\n";

static void disable_lua_file_io(lua_State *L)
{
    if (!replay.load_enable)
        return;
    CheckLua(L, lua_sandbox_dostring(L, replay_file_io_stubs), "replay_file_io_stubs");
}


/* @comment
 *     The loading items of open_lua_script and open_lua_script_for_mod need to be consistent.
 */
static void open_lua_standard_libraries(lua_State *L)
{
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
        // {LUA_JITLIBNAME,   luaopen_jit},
        // {LUA_FFILIBNAME,   luaopen_ffi},
        {NULL, NULL}
    };
    for (const luaL_Reg *lib = stdlib; lib->func != NULL; lib++) {
        lua_pushcfunction(L, lib->func);
        lua_pushstring(L, lib->name);
        lua_call(L, 1, 0);
    }
}

TbBool open_lua_script(LevelNumber lvnum)
{
    close_lua_script();
    Lvl_script = luaL_newstate();
    if (Lvl_script == NULL) {
        ERRORLOG("Failed to allocate Lua state");
        return false;
    }

    open_lua_standard_libraries(Lvl_script);
    // This edition: no FFI, text-only loaders (lua_sandbox.c)
    lua_sandbox_open(Lvl_script);
    disable_lua_functions(Lvl_script);
    disable_lua_file_io(Lvl_script);
    // After upstream's replay stubs, so replays read files and refuse writes as the game
    // they recorded did; this edition: see lua_readonly_io.c
    lua_make_files_read_only(Lvl_script);

    lua_set_random_seed(game.action_random_seed);

    reg_host_functions(Lvl_script);

    char default_path[4096];
    setLuaLibraryPath(Lvl_script, default_path, sizeof(default_path));

    char* fname = prepare_file_fmtpath(FGrp_FxData, "lua/init.lua");

    // Load and parse the Lua File
    if (!LbFileExists(fname)) {
        ERRORLOG("file %s missing",fname);
        close_lua_script();
        return false;
    }
    if (!CheckLua(Lvl_script, luaL_dofile(Lvl_script, fname), "global_lua_file")) {
        ERRORLOG("failed to load global lua script");
        close_lua_script();
        return false;
    }
    // The game's own library has kept what it needs of the debug library; campaigns, maps and
    // mods, loaded below, get only debug.traceback and debug.getinfo (lua_sandbox.c), and the
    // full module path: their level and campaign folders first
    lua_sandbox_close_debug(Lvl_script);
    restoreLuaDefaultPath(Lvl_script, default_path);
    setLuaPath(Lvl_script);

    fname = prepare_file_fmtpath(FGrp_CmpgConfig, "lua/init.lua");
    if (LbFileExists(fname))
    {
        if (!CheckLua(Lvl_script, luaL_dofile(Lvl_script, fname), "campaign_lua_file"))
        {
            ERRORLOG("failed to load campaign lua script");
        }
    }

    short fgroup = get_level_fgroup(lvnum);
    fname = prepare_file_fmtpath(fgroup, "map%05lu.lua", (unsigned long)lvnum);
    // Load and parse the Lua File
    if (LbFileExists(fname)) {
        if (!CheckLua(Lvl_script, luaL_dofile(Lvl_script, fname), "level_script_loading")) {
            ERRORLOG("failed to load level lua script");
        }
    }

    open_lua_script_for_mod_all(Lvl_script, lvnum);

    return true;
}


struct LuaSerialization {
    const char *function_name;
    const char *input;
    char *output;
    size_t length;
};

static int process_lua_serialization(lua_State *L)
{
    struct LuaSerialization *operation = lua_touserdata(L, 1);
    lua_getglobal(L, operation->function_name);
    if (!lua_isfunction(L, -1)) {
        return luaL_error(L, "%s is not a function", operation->function_name);
    }
    if (operation->input != NULL) {
        lua_pushlstring(L, operation->input, operation->length);
        lua_call(L, 1, 0);
        return 0;
    }
    lua_call(L, 0, 1);
    if (lua_type(L, -1) != LUA_TSTRING) {
        return luaL_error(L, "%s must return a string", operation->function_name);
    }
    const char *data = lua_tolstring(L, -1, &operation->length);
    size_t buffer_size = operation->length;
    if (buffer_size == 0) {
        buffer_size = 1;
    }
    operation->output = malloc(buffer_size);
    if (operation->output == NULL) {
        return luaL_error(L, "Failed to allocate Lua serialized data buffer");
    }
    memcpy(operation->output, data, operation->length);
    return 0;
}

static TbBool call_lua_serialization(struct LuaSerialization *operation)
{
    if (Lvl_script == NULL) {
        ERRORLOG("Lua state is not initialized");
        return false;
    }
    return CheckLua(Lvl_script, lua_cpcall(Lvl_script, process_lua_serialization, operation), operation->function_name);
}

char *lua_get_serialised_data(size_t *len)
{
    *len = 0;
    struct LuaSerialization operation = {"GetSerializedData", NULL, NULL, 0};
    if (!call_lua_serialization(&operation)) {
        free(operation.output);
        return NULL;
    }
    *len = operation.length;
    return operation.output;
}

TbBool lua_set_serialised_data(const char *data, size_t len)
{
    if (data == NULL) {
        ERRORLOG("Missing serialized Lua data");
        return false;
    }
    struct LuaSerialization operation = {"SetSerializedData", data, NULL, len};
    return call_lua_serialization(&operation);
}

void generate_lua_types_file()
{
    const char *filename = "native_types.lua";
    FILE *out = fopen(filename, "w");
    if (!out) {
        perror("Failed to open output file");
        return;
    }
    fprintf(out, "---@meta native_types\n");
    fprintf(out, "-- file not used by the game, but used for telling the IDE about what exists\n");
    fprintf(out, "-- this file contains fields changeble through cfg files, the one here in fxdata is the default\n");
    fprintf(out, "-- custom maps/campaigns can generate their own version, so all custom types are still recognized by the IDE\n\n");
    fprintf(out, "-- to generate one with the values specific to your map use the !luatypedump on the specific map you'd like the file for\n\n");

    #define GENERATE_ALIAS(alias_name, desc)            \
    do {                                                \
        int count = 0;                                  \
        for (int i = 0; desc[i].name != NULL; ++i) {    \
            if (desc[i].name[0] != '\0') ++count;       \
        }                                               \
        fprintf(out, "---@alias %s ", alias_name);      \
        if (count >= 90) {                              \
            fprintf(out, "string|");                    \
        }                                               \
        int written = 0;                                \
        for (int i = 0; desc[i].name != NULL; ++i) {    \
            if (desc[i].name[0] == '\0') continue;      \
            if (written > 0) fprintf(out, "|");         \
            fprintf(out, "\"%s\"", desc[i].name);       \
            ++written;                                  \
        }                                               \
        fprintf(out, "\n");                             \
    } while (0)

    #define GENERATE_FIELDS(class_name, desc)               \
    do {                                                 \
        fprintf(out, "---@class %s\n", class_name);       \
        for (int i = 0; desc[i].name != NULL; ++i) {      \
            if (desc[i].name[0] == '\0') continue; \
            fprintf(out, "---@field %s integer\n", desc[i].name); \
        }                                                \
        fprintf(out, "\n");                               \
        } while (0)

    // Generate sections
    GENERATE_ALIAS("creature_type", creature_desc);
    GENERATE_ALIAS("room_type", room_desc);
    GENERATE_ALIAS("power_kind", power_desc);
    GENERATE_ALIAS("trap_type", trap_desc);
    GENERATE_ALIAS("door_type", door_desc);
    GENERATE_ALIAS("object_type", object_desc);
    GENERATE_ALIAS("effect_generator_type", effectgen_desc);
    GENERATE_ALIAS("effect_element_type", effectelem_desc);
    GENERATE_ALIAS("effect_type", effect_desc);
    GENERATE_ALIAS("spell_type", spell_desc);
    GENERATE_ALIAS("slab_type", slab_desc);
    GENERATE_ALIAS("creature_state", creatrstate_desc);
    GENERATE_ALIAS("shot_type", shot_desc);
    fprintf(out, "\n");

    GENERATE_FIELDS("roomfields", room_desc);
    GENERATE_FIELDS("creaturefields", creature_desc);
    GENERATE_FIELDS("trapfields", trap_desc);
    GENERATE_FIELDS("doorfields", door_desc);

    // Cleanup
    #undef GENERATE_ALIAS
    #undef GENERATE_FIELDS

    fclose(out);
}


#ifdef __cplusplus
}
#endif
