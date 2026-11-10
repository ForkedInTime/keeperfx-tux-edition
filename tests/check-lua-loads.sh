#!/bin/sh
# Fails if the engine could load Lua code without going through the sandbox.
#
#   tests/check-lua-loads.sh
#
# Since stable 6072 (lua_sandbox.c) every chunk the engine loads for a map,
# campaign or mod is text only: LuaJIT bytecode let a crafted map reach memory
# and run native code. Lua's own loaders (luaL_loadfile, luaL_loadbuffer,
# lua_load, ...) take bytecode as readily as text, so one direct call added to
# the engine reopens that hole -- and the unit tests, which exercise the
# sandbox's own functions, would still pass. Upstream code is merged every week;
# upstream #5582 (multiplayer file checks) was such a change: it loaded mod
# scripts with luaL_loadfile and put its own module loader in package.loaders.
# Merged on 2026-11-10 with both going through lua_sandbox.c.
#
# The rules, checked on src/ (comments ignored):
#   1. No direct chunk loader outside src/lua_sandbox.c.
#   2. luaL_dofile only in a file that first redirects it to the sandbox
#      (lua_base.c's "#define luaL_dofile(L, fname) lua_dofile_on_disk(L, fname)").
#   3. The loader tables ("loaders", "searchers") are touched only where they are
#      today; a new place gets a person's look. If it loads text only through
#      lua_sandbox.c, add it to ALLOWED_LOADER_TABLES below.
#
# Exit status: 0 clean, 1 a load that bypasses the sandbox (or a new loader-table
# access to review).
set -eu
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

# file:count of "loaders"/"searchers" string literals allowed outside the sandbox.
# lua_base.c: add_lua_searcher_on_disk (2) and install_checksummed_lua_loaders (1),
# whose module loader loads through lua_sandbox_load_module.
ALLOWED_LOADER_TABLES="src/lua_base.c:3 src/lua_readonly_io.c:1"

failed=0
fail() {
    failed=1
    printf '%s\n' "$@"
}

# Drop comment lines and the code-free part of lines, so a comment that names a
# loader (lua_sandbox.h explains what it replaces) does not count.
code_lines() {
    git grep -nE "$1" -- 'src/*.c' 'src/*.cpp' 'src/*.h' 'src/**/*.c' 'src/**/*.cpp' 'src/**/*.h' \
        | grep -vE '^[^:]+:[0-9]+:[[:space:]]*(\*|//|/\*)' || true
}

direct="$(code_lines '\b(luaL_loadfilex?|luaL_loadbufferx?|luaL_loadstring|luaL_dostring|lua_load)[[:space:]]*\(' \
    | grep -v '^src/lua_sandbox\.c:' || true)"
if [ -n "$direct" ]; then
    fail "Lua code loaded directly, not through lua_sandbox.c (these loaders accept bytecode):" \
        "$(printf '%s\n' "$direct" | sed 's/^/  /')"
fi

for f in $(code_lines '\bluaL_dofile[[:space:]]*\(' | cut -d: -f1 | sort -u); do
    define_line="$(grep -n '^#define luaL_dofile(L, fname) lua_dofile_on_disk(L, fname)' "$f" | head -1 | cut -d: -f1)"
    first_use="$(grep -nE '\bluaL_dofile[[:space:]]*\(' "$f" | grep -vE '^[0-9]+:[[:space:]]*(\*|//|/\*|#define)' | head -1 | cut -d: -f1)"
    # Only the redirect itself: nothing is loaded through it.
    [ -n "$first_use" ] || continue
    if [ -z "$define_line" ] || [ "$first_use" -lt "$define_line" ]; then
        fail "$f calls luaL_dofile without first redirecting it to the sandbox (line $first_use)."
    fi
done

tables="$(code_lines '"(loaders|searchers)"' | grep -v '^src/lua_sandbox\.c:' | cut -d: -f1 | sort | uniq -c \
    | awk '{print $2 ":" $1}')"
for entry in $tables; do
    case " $ALLOWED_LOADER_TABLES " in
        *" $entry "*) ;;
        *) fail "New access to package.loaders/searchers in ${entry%:*} (now ${entry##*:} uses):" \
                "$(code_lines '"(loaders|searchers)"' | grep "^${entry%:*}:" | sed 's/^/  /')" \
                "  Check it loads text only through lua_sandbox.c, then update ALLOWED_LOADER_TABLES in $0." ;;
    esac
done

if [ "$failed" -ne 0 ]; then
    echo
    echo "Every Lua chunk must load through lua_sandbox.c (text only). See the header of $0."
    exit 1
fi
echo "Lua code loads only through the sandbox."
