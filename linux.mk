include version.mk

# A recipe that fails must not leave its half-written target behind looking
# finished (an extracted header, a downloaded archive).
.DELETE_ON_ERROR:

BUILD_NUMBER ?= $(VER_BUILD)
VER_SUFFIX ?= Prototype
VER_STRING = $(strip $(VER_MAJOR).$(VER_MINOR).$(VER_RELEASE).$(BUILD_NUMBER) $(VER_SUFFIX))

MKDIR ?= mkdir -p
STRIP ?= strip
ECHO ?= echo
MV ?= mv -f

KFX_SOURCES = \
src/actionpt.c \
src/api.c \
src/ariadne.c \
src/ariadne_edge.c \
src/ariadne_findcache.c \
src/ariadne_naviheap.c \
src/ariadne_navitree.c \
src/ariadne_points.c \
src/ariadne_regions.c \
src/ariadne_tringls.c \
src/ariadne_update.c \
src/ariadne_wallhug.c \
src/bflib_basics.c \
src/bflib_coroutine.c \
src/bflib_cpu.c \
src/bflib_crash.c \
src/bflib_datetm.cpp \
src/bflib_dernc.c \
src/bflib_enet.cpp \
src/net_portforward.cpp \
src/bflib_fileio.c \
src/bflib_filelst.c \
src/bflib_fmvids.cpp \
src/bflib_guibtns.c \
src/bflib_input_joyst.cpp \
src/bflib_inputctrl.cpp \
src/bflib_keybrd.c \
src/bflib_main.cpp \
src/bflib_math.c \
src/bflib_mouse.cpp \
src/bflib_mshandler.cpp \
src/bflib_mspointer.cpp \
src/bflib_netsession.c \
src/bflib_netsp.cpp \
src/net_exchange_common.c \
src/net_exchange_gameplay.c \
src/net_main.c \
src/net_lobby.c \
src/bflib_planar.c \
src/kfx/renderer/software/bflib_render.c \
src/kfx/renderer/software/bflib_render_gpoly.c \
src/kfx/renderer/software/bflib_render_trig.c \
src/bflib_sndlib.cpp \
src/bflib_sound.c \
src/bflib_sprfnt.c \
src/bflib_string.c \
src/bflib_text.c \
src/bflib_video.c \
src/kfx/renderer/software/bflib_vidraw.c \
src/kfx/renderer/software/bflib_vidraw_spr_norm.c \
src/kfx/renderer/software/bflib_vidraw_spr_onec.c \
src/kfx/renderer/software/bflib_vidraw_spr_remp.c \
src/kfx/renderer/software/SwDrawTarget.c \
src/kfx/renderer/software/SwDisplaySurface.c \
src/kfx/renderer/software/SwZoomBoxTiles.c \
src/kfx/renderer/RendererSettings.c \
src/kfx/ai/TargetSearchProbe.c \
src/button_snapping.c \
src/config.c \
src/config_campaigns.c \
src/config_creature.c \
src/config_crtrmodel.c \
src/config_crtrstates.c \
src/config_keeperfx.c \
src/config_lenses.c \
src/config_magic.c \
src/config_mods.c \
src/config_objects.c \
src/config_players.c \
src/config_powerhands.c \
src/config_rules.c \
src/config_sounds.c \
src/config_settings.c \
src/config_slabsets.c \
src/config_strings.c \
src/config_terrain.c \
src/config_cubes.c \
src/config_textures.c \
src/config_translation.c \
src/config_trapdoor.c \
src/config_spritecolors.c \
src/console_cmd.c \
src/custom_sprites.c \
src/custom_zip.c \
src/creature_battle.c \
src/creature_control.c \
src/creature_graphics.c \
src/creature_groups.c \
src/creature_instances.c \
src/creature_jobs.c \
src/creature_senses.c \
src/creature_states.c \
src/creature_states_barck.c \
src/creature_states_combt.c \
src/creature_states_gardn.c \
src/creature_states_guard.c \
src/creature_states_hero.c \
src/creature_states_lair.c \
src/creature_states_mood.c \
src/creature_states_pray.c \
src/creature_states_prisn.c \
src/creature_states_rsrch.c \
src/creature_states_scavn.c \
src/creature_states_spdig.c \
src/creature_states_tortr.c \
src/creature_states_train.c \
src/creature_states_tresr.c \
src/creature_states_wrshp.c \
src/cursor_tag.c \
src/dungeon_data.c \
src/dungeon_stats.c \
src/engine_arrays.c \
src/engine_camera.c \
src/local_camera.c \
src/engine_lenses.c \
src/engine_redraw.c \
src/engine_render.c \
src/engine_render_data.cpp \
src/engine_textures.c \
src/front_credits.c \
src/front_easter.c \
src/front_fmvids.c \
src/front_highscore.c \
src/front_input.c \
src/front_landview.c \
src/front_landview_multiplayer.c \
src/front_lvlstats.c \
src/front_lvlstats_data.cpp \
src/front_network.c \
src/front_simple.c \
src/front_torture.c \
src/front_torture_data.cpp \
src/frontend.cpp \
src/frontmenu_select.c \
src/frontmenu_ingame_evnt.c \
src/frontmenu_ingame_evnt_data.cpp \
src/frontmenu_ingame_map.c \
src/frontmenu_ingame_opts.c \
src/frontmenu_ingame_opts_data.cpp \
src/frontmenu_ingame_tabs.c \
src/frontmenu_ingame_tabs_data.cpp \
src/frontmenu_net.c \
src/frontmenu_net_data.cpp \
src/frontmenu_options.c \
src/frontmenu_options_data.cpp \
src/frontmenu_saves.c \
src/frontmenu_saves_data.cpp \
src/frontmenu_select_data.cpp \
src/frontmenu_specials.c \
src/game_heap.c \
src/game_legacy.c \
src/game_loop.c \
src/game_lghtshdw.c \
src/game_merge.c \
src/game_saves.c \
src/game_update.cpp \
src/gui_boxmenu.c \
src/gui_draw.c \
src/gui_frontbtns.c \
src/gui_frontmenu.c \
src/gui_msgs.c \
src/gui_parchment.c \
src/gui_soundmsgs.cpp \
src/gui_tooltips.c \
src/gui_topmsg.c \
src/gui_vscroll.c \
src/highscores.c \
src/kjm_input.c \
src/lens_api.c \
src/config_effects.c \
src/kfx_memory.c \
src/kfx/platform/PlatformManager.cpp \
src/kfx/platform/PlatformLinux.cpp \
src/kfx/platform/WindowSystemSDL.cpp \
src/kfx/renderer/ITextRenderer.cpp \
src/kfx/renderer/IUIRenderer.cpp \
src/kfx/renderer/RendererManager.cpp \
src/kfx/renderer/RendererSoftware.cpp \
src/kfx/lense/DisplacementEffect.cpp \
src/kfx/lense/FlyeyeEffect.cpp \
src/kfx/lense/LensEffect.cpp \
src/kfx/lense/LensManager.cpp \
src/kfx/lense/LuaLensEffect.cpp \
src/kfx/lense/MistEffect.cpp \
src/kfx/lense/OverlayEffect.cpp \
src/kfx/lense/PaletteEffect.cpp \
src/kfx/platform/GLContextSDL.cpp \
src/kfx/renderer/PaletteTransform.cpp \
src/kfx/renderer/RenderGraph.cpp \
src/kfx/renderer/RenderTaskProducerRegistry.cpp \
src/kfx/renderer/RenderThreadManager.cpp \
src/kfx/renderer/RendererBridge_UI.cpp \
src/kfx/renderer/RendererFrameCounter.cpp \
src/kfx/renderer/RendererOpenGL.cpp \
src/kfx/renderer/RendererThread.cpp \
src/kfx/renderer/TileAtlasPacker.cpp \
src/kfx/renderer/backends/SoftwareCursorLayer.cpp \
src/kfx/renderer/backends/SoftwareWorldViewRenderer.cpp \
src/kfx/renderer/opengl/GLCursorLayer.cpp \
src/kfx/renderer/opengl/GLFunctions.cpp \
src/kfx/renderer/opengl/GLImagePresentPass.cpp \
src/kfx/renderer/opengl/GLMapFadePass.cpp \
src/kfx/renderer/opengl/GLPaletteIndexLookup.cpp \
src/kfx/renderer/opengl/GLResourceMapper.cpp \
src/kfx/renderer/opengl/GLSpriteAtlas.cpp \
src/kfx/renderer/opengl/GLTextRenderer.cpp \
src/kfx/renderer/opengl/GLTileAtlas.cpp \
src/kfx/renderer/opengl/GLUIRenderer.cpp \
src/kfx/renderer/opengl/GLWorldViewRenderer.cpp \
src/kfx/renderer/opengl/GLZoomBoxTilesPass.cpp \
src/kfx/ui/GameUI.cpp \
src/light_data.c \
src/lua_api.c \
src/lua_api_lens.c \
src/lua_api_map.c \
src/lua_api_player.c \
src/lua_api_room.c \
src/lua_api_things.c \
src/lua_api_slabs.c \
src/lua_api_sound.c \
src/lua_base.c \
src/lua_api_camera.c \
src/lua_cfg_funcs.c \
src/lua_params.c \
src/lua_triggers.c \
src/lua_utils.c \
src/lvl_filesdk1.c \
src/lvl_script.c \
src/lvl_script_commands.c \
src/lvl_script_commands_old.c \
src/lvl_script_lib.c \
src/lvl_script_conditions.c \
src/lvl_script_value.c \
src/magic_powers.c \
src/main.cpp \
src/main_game.c \
src/map_blocks.c \
src/map_columns.c \
src/map_ceiling.c \
src/map_data.c \
src/map_events.c \
src/map_locations.c \
src/map_utils.c \
src/moonphase.c \
src/net_game.c \
src/net_holepunch.c \
src/net_matchmaking.c \
src/net_lan.c \
src/net_input_lag.c \
src/net_resync.cpp \
src/packets.c \
src/packets_cheats.c \
src/packets_input.c \
src/packets_misc.c \
src/replay.c \
src/player_compchecks.c \
src/player_compevents.c \
src/player_complookup.c \
src/config_compp.c \
src/player_compprocs.c \
src/player_comptask.c \
src/player_computer.c \
src/player_computer_data.cpp \
src/player_data.c \
src/player_instances.c \
src/player_utils.c \
src/power_hand.c \
src/power_process.c \
src/power_specials.c \
src/room_data.c \
src/room_entrance.c \
src/room_garden.c \
src/room_graveyard.c \
src/room_jobs.c \
src/room_lair.c \
src/room_library.c \
src/room_list.c \
src/room_scavenge.c \
src/room_treasure.c \
src/room_util.c \
src/room_workshop.c \
src/roomspace.c \
src/roomspace_detection.c \
src/scrcapt.c \
src/slab_data.c \
src/sounds.c \
src/sound_manager.cpp \
src/spdigger_stack.c \
src/spritesheet.cpp \
src/tasks_list.c \
src/thing_corpses.c \
src/thing_creature.c \
src/thing_data.c \
src/thing_doors.c \
src/thing_effects.c \
src/thing_factory.c \
src/thing_list.c \
src/thing_navigate.c \
src/thing_objects.c \
src/thing_physics.c \
src/thing_shots.c \
src/thing_stats.c \
src/thing_traps.c \
src/timer.c \
src/value_util.c \
src/vidfade.c \
src/vidmode_data.cpp \
src/vidmode.c \
src/front_landview_textbox.c \
src/logging.c \
src/net_spectator.c \
src/observer.c \
src/lua_readonly_io.c \
src/lua_sandbox.c \
src/kernel_memtracker.c \
src/kfx/platform/FileSystemLb.cpp \
src/kfx/save/core/save_codec.cpp \
src/kfx/save/core/save_config_overlay.cpp \
src/kfx/save/core/save_convert.cpp \
src/kfx/save/core/save_decode.cpp \
src/kfx/save/core/save_encode.cpp \
src/kfx/save/core/save_inspect.cpp \
src/kfx/save/core/save_migrate.cpp \
src/kfx/save/core/save_names.cpp \
src/kfx/save/core/save_recfile.cpp \
src/kfx/save/core/save_resync.cpp \
src/kfx/save/core/save_schema.cpp \
src/kfx/save/core/save_schema_file.cpp \
src/kfx/save/core/schema/schema_all.c \
src/kfx/save/core/schema/schema_config.c \
src/kfx/save/core/schema/schema_dungeon.c \
src/kfx/save/core/schema/schema_game.c \
src/kfx/save/core/schema/schema_misc.c \
src/kfx/save/core/schema/schema_script.c \
src/kfx/save/core/schema/schema_things.c \
src/kfx/save/core/schema/schema_world.c \
src/kfx/save/SaveCatalogue.cpp \
src/kfx/save/SaveContinue.cpp \
src/kfx/save/SaveGameFile.cpp \
src/kfx/save/SaveManager.cpp \
src/kfx/save/SaveRecordFiles.cpp \
src/kfx/save/SaveReplayHeader.cpp \
src/kfx/save/SaveUnionSelectors.cpp \
src/replay_rewind.c \
src/roomspace_input.c \
src/save_trash.c \
src/legacy_progress_lp64.c \
src/kfx/save/SaveCampaignProgress.cpp \
src/local_possession.c \
src/net_checksum_turn.c \
src/net_filecheck.c

KFX_C_SOURCES = $(filter %.c,$(KFX_SOURCES))
KFX_CXX_SOURCES = $(filter %.cpp,$(KFX_SOURCES))
KFX_C_OBJECTS = $(patsubst src/%.c,obj/%.o,$(KFX_C_SOURCES))
KFX_CXX_OBJECTS = $(patsubst src/%.cpp,obj/%.o,$(KFX_CXX_SOURCES))
KFX_OBJECTS = $(KFX_C_OBJECTS) $(KFX_CXX_OBJECTS)
# glad, the GL loader upstream's OpenGL backend resolves every gl* call through.
# Vendored under deps/, built like any other C object; no -lGL, the loader gets
# its entry points from SDL_GL_GetProcAddress at runtime.
GLAD_OBJECTS = obj/glad/glad.o

# libcurl (the lobby's websocket). By default the team's prebuilt static library
# from kfx-deps, which the bundled builds (AppImage, Flatpak, portable) ship.
# SYSTEM_CURL=1 links the distribution's libcurl instead: the prebuilt is
# compiled against OpenSSL 3 and calls its ENGINE API, which OpenSSL 4 removed,
# so it no longer links where the system has OpenSSL 4 (Arch since 2026-11-10).
# The AUR package sets it; the engine itself uses OpenSSL only through libcurl.
ifeq ($(SYSTEM_CURL),1)
ifneq ($(shell pkg-config --exists libcurl && echo yes),yes)
$(error SYSTEM_CURL=1 needs libcurl's pkg-config file (libcurl.pc): install your distribution's libcurl development package)
endif
CURL_INCLUDES = $(shell pkg-config --cflags-only-I libcurl)
CURL_LIBS = $(shell pkg-config --libs-only-l libcurl)
CURL_PREBUILT =
else
CURL_INCLUDES = -Ideps/libcurl/include
CURL_LIBS = -Ldeps/libcurl/lib -lcurl -lssl -lcrypto -lzstd
CURL_PREBUILT = deps/libcurl/lib/libcurl.a
endif

KFX_INCLUDES = \
	-Isrc \
	-Ideps/centijson/include \
	-Ideps/centitoml \
	-Ideps/astronomy/include \
	-Ideps/enet6/include \
	$(CURL_INCLUDES) \
	$(shell pkg-config --cflags-only-I luajit) \
	-isystem deps/glad/include \
	$(shell pkg-config --cflags-only-I libavformat)

# -Werror=unused-result: a discarded return value is promoted to a build error.
# Nine of the thirteen launcher bugs found in the 2026-08-05 review shared this
# one root cause -- QFile::copy, QDir::rename, SDL_TryLockMutex, all compiling
# clean while a failure silently became a success. The engine builds with zero
# such warnings today, so this costs nothing and keeps the class out.
KFX_CFLAGS += -g -DDEBUG -DBFDEBUG_LEVEL=0 -O3 -march=x86-64 $(KFX_INCLUDES) -Wall -Wextra -Wno-error -Werror=unused-result -Wno-unused-parameter -Wno-absolute-value -Wno-unknown-pragmas -Wno-format-truncation -Wno-sign-compare -MMD -MP
KFX_CXXFLAGS += -g -DDEBUG -DBFDEBUG_LEVEL=0 -O3 -march=x86-64 $(KFX_INCLUDES) -Wall -Wextra -Wno-error -Werror=unused-result -Wno-unused-parameter -Wno-unknown-pragmas -Wno-format-truncation -Wno-sign-compare -MMD -MP

# SANITIZE=1: AddressSanitizer + UndefinedBehaviorSanitizer build, for the local
# regression pass (tests/sanitize-regression.sh) -- not for release. UBSan is
# built in recover mode so a run REPORTS every finding rather than dying at the
# first one; the report is the deliverable. Objects land in the same obj/ as a
# normal build, so always build from clean -- the script does.
ifeq ($(SANITIZE),1)
KFX_SAN_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer -O1 -fno-common
KFX_CFLAGS += $(KFX_SAN_FLAGS)
KFX_CXXFLAGS += $(KFX_SAN_FLAGS)
KFX_LDFLAGS += -fsanitize=address,undefined
endif

KFX_LDFLAGS += \
	-g \
	-rdynamic \
	-Wall -Wextra -Wno-error \
	-Ldeps/astronomy -lastronomy \
	-Ldeps/centijson -ljson \
	-Ldeps/enet6 -lenet6 \
	$(shell pkg-config --libs-only-l sdl3) \
	$(shell pkg-config --libs-only-l sdl3-mixer) \
	$(shell pkg-config --libs-only-l sdl3-image) \
	$(shell pkg-config --libs-only-l libavformat) \
	$(shell pkg-config --libs-only-l libavcodec) \
	$(shell pkg-config --libs-only-l libswresample) \
	$(shell pkg-config --libs-only-l libswscale) \
	$(shell pkg-config --libs-only-l libavutil) \
	$(shell pkg-config --libs-only-l openal) \
	$(shell pkg-config --libs-only-l luajit) \
	$(shell pkg-config --libs-only-l spng) \
	$(shell pkg-config --libs-only-l minizip) \
	$(shell pkg-config --libs-only-l zlib) \
	-lminiupnpc \
	-lnatpmp \
	$(CURL_LIBS) \
	-ldl

TOML_SOURCES = \
	deps/centitoml/toml_api.c

TOML_OBJECTS = $(patsubst deps/centitoml/%.c,obj/centitoml/%.o,$(TOML_SOURCES))

TOML_INCLUDES = \
	-Isrc \
	-Ideps/centijson/include

TOML_CFLAGS += -O3 -march=x86-64 $(TOML_INCLUDES) -Wall -Wextra -Wno-error -Wno-unused-parameter

ifeq ($(ENABLE_LTO), 1)
KFX_CFLAGS += -flto
KFX_CXXFLAGS += -flto
KFX_LDFLAGS += -flto=auto
TOML_CFLAGS += -flto
endif

# The TOML parser reads every config file, mods' included, so the sanitizer build checks it too.
ifeq ($(SANITIZE),1)
TOML_CFLAGS += $(KFX_SAN_FLAGS)
endif

# All downloaded dependencies must be unpacked before any object is compiled.
# Otherwise a parallel build (make -jN) can start compiling a source that
# includes a not-yet-extracted dependency header (e.g. <enet6/enet.h>) and fail
# on the first run. Used as an order-only prerequisite of every object below.
DEPS_EXTRACTED = \
	deps/centijson/include/json.h \
	deps/astronomy/include/astronomy.h \
	deps/enet6/include/enet6/enet.h \
	$(CURL_PREBUILT)

all: bin/keeperfx

clean:
	rm -rf obj bin src/ver_defs.h deps/astronomy deps/centijson deps/enet6 deps/libcurl
	rm -f deps/libcurl-lin64.tar.gz

.PHONY: all clean

# `make -f linux.mk print-VAR` prints one variable's expanded value and builds
# nothing. packaging/linux-only/prove-equivalence.sh reads the source lists and
# compiler flags this way, so its check always uses exactly what the build uses.
print-%:
	$(info $($*))
	@:

bin/keeperfx: $(KFX_OBJECTS) $(TOML_OBJECTS) $(GLAD_OBJECTS) $(CURL_PREBUILT) | bin
	$(CXX) -o $@ $(KFX_OBJECTS) $(TOML_OBJECTS) $(GLAD_OBJECTS) $(KFX_LDFLAGS)

$(KFX_C_OBJECTS): obj/%.o: src/%.c src/ver_defs.h | obj $(DEPS_EXTRACTED)
	$(MKDIR) $(dir $@)
	$(CC) $(KFX_CFLAGS) -c $< -o $@

$(GLAD_OBJECTS): obj/glad/%.o: deps/glad/src/%.c | obj
	$(MKDIR) $(dir $@)
	$(CC) $(KFX_CFLAGS) -Wno-pedantic -c $< -o $@

$(KFX_CXX_OBJECTS): obj/%.o: src/%.cpp src/ver_defs.h | obj $(DEPS_EXTRACTED)
	$(MKDIR) $(dir $@)
	$(CXX) $(KFX_CXXFLAGS) -c $< -o $@

$(TOML_OBJECTS): obj/centitoml/%.o: deps/centitoml/%.c | obj/centitoml $(DEPS_EXTRACTED)
	$(CC) $(TOML_CFLAGS) -c $< -o $@

bin obj deps/astronomy deps/centijson deps/enet6 deps/libcurl obj/centitoml:
	$(MKDIR) $@

src/actionpt.c: deps/centijson/include/json.h
src/api.c: deps/centijson/include/json.h
src/bflib_enet.cpp: deps/enet6/include/enet6/enet.h
src/moonphase.c: deps/astronomy/include/astronomy.h
src/net_holepunch.c: deps/enet6/include/enet6/enet.h
ifneq ($(SYSTEM_CURL),1)
src/net_matchmaking.c: deps/libcurl/include/curl/curl.h
endif
deps/centitoml/toml_api.c: deps/centijson/include/json.h
deps/centitoml/toml_conv.c: deps/centijson/include/json.h

# -MMD -MP (above) makes each compile emit a .d file listing the headers it
# actually pulled in; without this, make only ever looks at a .c/.cpp file's
# own mtime, so a header-only change (e.g. a struct gaining a field) left every
# .o that transitively includes it looking "up to date" after a plain
# `git checkout`/branch switch -- silently linking mismatched struct layouts
# across translation units. Reproduced once: incrementally building 862029101
# then e1e47edb3 in the same obj/ produced a binary that corrupted netstate
# every turn; a from-scratch build of e1e47edb3 was clean. `make clean` before
# switching commits was the only prior mitigation, easy to forget.
#
# Does NOT close the gap when DISTCC_HOSTS routes a compile to the remote
# host: distcc does not always bring the .d file back with the .o (confirmed
# missing for ~19% of objects in one run here). Still always `make clean`
# before an incremental build after switching commits when distributing.
-include $(KFX_OBJECTS:.o=.d) $(TOML_OBJECTS:.o=.d) $(GLAD_OBJECTS:.o=.d)

# Prebuilt dependencies from dkfans/kfx-deps. `curl -f` makes an HTTP error fail
# instead of saving the error page as the archive (which was then never fetched
# again, so every later build failed -- in an AUR build directory, until a clean
# build). The download goes to a temporary name and is moved into place only
# once its SHA-256 matches, so a truncated or substituted archive is never used.
# A new upstream release of one of these means a new URL here, and a new hash:
#   curl -fLsS -o x.tar.gz <url> && sha256sum x.tar.gz
define fetch_dep
	curl -fLsS --retry 3 -o $@.part "$(1)"
	echo "$(2)  $@.part" | sha256sum -c --quiet - || { rm -f $@.part; echo "linux.mk: checksum mismatch for $(1)" >&2; exit 1; }
	mv -f $@.part $@
endef

deps/astronomy-lin64.tar.gz:
	$(call fetch_dep,https://github.com/dkfans/kfx-deps/releases/download/20250418/astronomy-lin64.tar.gz,7d2bedd5e97035b26257c6f593eaf24a853bcd72a1db0da4aa56706bdfbc0e22)

deps/astronomy/include/astronomy.h: deps/astronomy-lin64.tar.gz | deps/astronomy
	tar xzmf $< -C deps/astronomy

deps/centijson-lin64.tar.gz:
	$(call fetch_dep,https://github.com/dkfans/kfx-deps/releases/download/20250418/centijson-lin64.tar.gz,ba42b5146140978da909bff4fc89fb09c064d315c4db76e56dfd51284ddd329f)

deps/centijson/include/json.h: deps/centijson-lin64.tar.gz | deps/centijson
	tar xzmf $< -C deps/centijson

deps/enet6-lin64.tar.gz:
	$(call fetch_dep,https://github.com/dkfans/kfx-deps/releases/download/20260213/enet6-lin64.tar.gz,04615d63f8e540604c195f2ea40076ef51903031695104a95a1c613847e772f9)

deps/enet6/include/enet6/enet.h: deps/enet6-lin64.tar.gz | deps/enet6
	tar xzmf $< -C deps/enet6

deps/libcurl-lin64.tar.gz:
	$(call fetch_dep,https://github.com/dkfans/kfx-deps/releases/download/20260310/libcurl-lin64.tar.gz,5aaac722ec4a7cf24c4c1b02279739661e9cd32e858644db5ddae6c11b8b20e9)

deps/libcurl/lib/libcurl.a: deps/libcurl-lin64.tar.gz | deps/libcurl
	tar xzmf $< -C deps/libcurl

deps/libcurl/include/curl/curl.h: deps/libcurl/lib/libcurl.a

src/ver_defs.h: version.mk
	$(ECHO) "#define VER_MAJOR   $(VER_MAJOR)" > $@.swp
	$(ECHO) "#define VER_MINOR   $(VER_MINOR)" >> $@.swp
	$(ECHO) "#define VER_RELEASE $(VER_RELEASE)" >> $@.swp
	$(ECHO) "#define VER_BUILD   $(BUILD_NUMBER)" >> $@.swp
	$(ECHO) "#define VER_STRING  \"$(VER_STRING)\"" >> $@.swp
	$(ECHO) "#define PACKAGE_SUFFIX  \"$(VER_SUFFIX)\"" >> $@.swp
	$(ECHO) "#define GIT_REVISION  \"$(shell git describe  --always)\"" >> $@.swp
	$(MV) $@.swp $@
