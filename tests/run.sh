#!/bin/sh
# Builds and runs the unit tests. They do not link the engine, but the
# file-enumerator test compiles the real platform layer, so it needs the SDL3
# headers (load packaging/ci/build-sdl3.sh --env first, as a build does) and the
# generated src/ver_defs.h, which is made here if it is missing.
#
#   tests/run.sh
#
# SANITIZE=1 builds the tests with AddressSanitizer + UBSan, so the same
# assertions also police memory errors and undefined behaviour. CI
# (sanitize-build.yml) runs both modes; the flags change nothing about what is
# asserted.
#
# Exits non-zero if any build or any test fails.
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$ROOT_DIR/bin"
[ -f "$ROOT_DIR/src/ver_defs.h" ] || make -s -C "$ROOT_DIR" -f linux.mk src/ver_defs.h

EXTRA_FLAGS=""
if [ "${SANITIZE:-0}" = "1" ]; then
    EXTRA_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g"
fi

# Pure track-mapping logic, straight out of the header.
g++ -std=c++17 -Wall -Wextra $EXTRA_FLAGS -o "$ROOT_DIR/bin/test_music_index" "$ROOT_DIR/tests/test_music_index.cpp"
"$ROOT_DIR/bin/test_music_index"

# The software rasteriser's 32-bit carry/rotate helpers. They are static inline in
# a file that needs the whole engine, so the two functions are cut out of the real
# source: the test checks the code that ships, not a copy of it.
TRIG_SRC="$ROOT_DIR/src/kfx/renderer/software/bflib_render_trig.c"
awk '/^static inline .*__CFADDL__\(/,/^}/; /^static inline .*__ROL4__\(/,/^}/' "$TRIG_SRC" \
	> "$ROOT_DIR/bin/render_trig_helpers.inc"
if [ "$(grep -c '^static inline' "$ROOT_DIR/bin/render_trig_helpers.inc")" != 2 ]; then
	echo "tests/run.sh: __CFADDL__/__ROL4__ not found in $TRIG_SRC" >&2
	exit 1
fi
g++ -std=c++17 -Wall -Wextra $EXTRA_FLAGS -I"$ROOT_DIR/bin" \
	-o "$ROOT_DIR/bin/test_render_trig_carry" "$ROOT_DIR/tests/test_render_trig_carry.cpp"
"$ROOT_DIR/bin/test_render_trig_carry"

# Importing fx1contn.sav as 64-bit builds wrote it (campaign_flags as 8-byte
# longs) into the int32_t IntralevelData. The decoder is the real
# src/legacy_progress_lp64.c, compiled against the engine's own headers. Checked
# once by hand against a file written by the old struct itself (compiled from the
# v1.4.0.5660 tag): every field, flag and ensign override came back where it was.
cc -std=gnu17 -Wall -Wextra -Wno-unused-parameter $EXTRA_FLAGS -I"$ROOT_DIR/src" $(pkg-config --cflags sdl3) \
	-o "$ROOT_DIR/bin/test_legacy_progress" "$ROOT_DIR/tests/test_legacy_progress.c" "$ROOT_DIR/src/legacy_progress_lp64.c"
"$ROOT_DIR/bin/test_legacy_progress"

# Level and campaign Lua may read files but not change them, nor load native
# code through require (src/lua_readonly_io.c). The state is built the way
# open_lua_script() builds it and attacked the way a hostile map would; the
# game's own Lua (bundled serialiser, bytecode restore, debug functions) must
# keep working. Links the real bflib_fileio.c for its case-insensitive lookup.
cc -Wall -Wextra -Wno-unused-parameter $EXTRA_FLAGS -I"$ROOT_DIR/src" $(pkg-config --cflags sdl3 luajit) \
	-c "$ROOT_DIR/src/lua_readonly_io.c" -o "$ROOT_DIR/bin/lua_readonly_io_for_tests.o"
# The real Linux directory enumerator, run against a real temporary directory.
# It lives in the platform layer since upstream #5104/#5107 (it was linux.cpp
# before); the enumerator needs SDL3 headers to compile and the tests stub the
# engine symbols PlatformManager/PlatformLinux reference but the tests never call.
#
# PlatformLinux.cpp carries the game's process entry point, so it is compiled to
# its own object with main() renamed away -- linking it whole would collide with
# the test's own main(). The rename CANNOT move to the combined command below:
# -Dmain= applies to every translation unit it is passed, so it would rename the
# test's entry point too and the binary would have none. This separate-object
# step existed for src/linux.cpp and was dropped when upstream's platform-seam
# refactor moved the file; the tests stopped linking (silently, in a weekly job)
# from 2026-08-24 until this was restored.
g++ -std=c++17 -Wall -Wextra $EXTRA_FLAGS -I"$ROOT_DIR/src" $(pkg-config --cflags sdl3) \
	-Dmain=keeperfx_platform_main_unused \
	-c "$ROOT_DIR/src/kfx/platform/PlatformLinux.cpp" \
	-o "$ROOT_DIR/bin/platform_linux_for_tests.o"
# PlatformManager owns the file system the save code reads through (upstream
# #5424), so its FileSystemLb.cpp is linked as well.
# The enumerator finds its directory through LbFileCaseInsensitivePath(), so the
# real bflib_fileio.c is linked too (it is C, so it gets its own object), with
# the -Wno-format-truncation linux.mk builds it with.
cc -Wall -Wextra -Wno-format-truncation $EXTRA_FLAGS -I"$ROOT_DIR/src" $(pkg-config --cflags sdl3) \
	-c "$ROOT_DIR/src/bflib_fileio.c" \
	-o "$ROOT_DIR/bin/bflib_fileio_for_tests.o"
g++ -std=c++17 -Wall -Wextra $EXTRA_FLAGS -I"$ROOT_DIR/src" $(pkg-config --cflags sdl3) \
	-o "$ROOT_DIR/bin/test_file_find" \
	"$ROOT_DIR/tests/test_file_find.cpp" \
	"$ROOT_DIR/src/kfx/platform/PlatformManager.cpp" \
	"$ROOT_DIR/src/kfx/platform/FileSystemLb.cpp" \
	"$ROOT_DIR/bin/platform_linux_for_tests.o" \
	"$ROOT_DIR/bin/bflib_fileio_for_tests.o" \
	$(pkg-config --libs sdl3)
"$ROOT_DIR/bin/test_file_find"

g++ -std=c++17 -Wall -Wextra $EXTRA_FLAGS -I"$ROOT_DIR/src" $(pkg-config --cflags sdl3 luajit) \
	-o "$ROOT_DIR/bin/test_lua_readonly" \
	"$ROOT_DIR/tests/test_lua_readonly.cpp" \
	"$ROOT_DIR/bin/lua_readonly_io_for_tests.o" \
	"$ROOT_DIR/src/kfx/platform/PlatformManager.cpp" \
	"$ROOT_DIR/src/kfx/platform/FileSystemLb.cpp" \
	"$ROOT_DIR/bin/platform_linux_for_tests.o" \
	"$ROOT_DIR/bin/bflib_fileio_for_tests.o" \
	$(pkg-config --libs sdl3 luajit)
"$ROOT_DIR/bin/test_lua_readonly" "$ROOT_DIR"

# The test above checks the sandbox's own loaders; this checks nothing in the
# engine goes around them. A direct luaL_loadfile and the like accepts bytecode,
# so one such call added by an upstream merge (as #5582 does) fails here.
sh "$ROOT_DIR/tests/check-lua-loads.sh"

# The save format (upstream #5424): upstream's own codec and file tests, built
# for this edition's 64-bit target. They round-trip a real struct Game through
# the field tables and compare digests, so a table that reads differently here
# than on upstream's 32-bit Windows build fails. SDL_INCLUDES must not be empty,
# or build.sh downloads the headers instead of using the ones found here.
SDL_INCLUDES="$(pkg-config --cflags-only-I sdl3) -I$ROOT_DIR/src" \
	CFLAGS_EXTRA="$EXTRA_FLAGS" LDFLAGS_EXTRA="$EXTRA_FLAGS" \
	bash "$ROOT_DIR/tests/save/build.sh" "$ROOT_DIR/bin/test_save_format"
"$ROOT_DIR/bin/test_save_format"
"$ROOT_DIR/bin/test_save_format_records"
