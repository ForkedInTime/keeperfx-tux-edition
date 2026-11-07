# Changelog — KeeperFX Linux Alpha

This tracks the changes in *this* fork on top of the KeeperFX team's `master`.
Version numbers follow the engine build (`<major>.<minor>.<release>.<build>`), with
`alpha` appended on the alpha channel and nothing appended on the stable one.

## 1.4.0.6103 — 2026-11-06 — stable

**Replaces 1.4.0.6098.** Three of the KeeperFX team's fixes, taken early, with their authors.

- **Creatures that hate each other fight again until you separate them.** When a creature sleeping in its lair
  found its lair enemy in the same room (a Bile Demon and a Skeleton, a Spider and a Fly, a Warlock and a
  Vampire), the mentor said your creatures were fighting among themselves, and the fight stopped half a second
  later with no damage done: the creature attacked dropped out at once. Every stable since 1.4.0.6062 did this.
  It came from a change of the team's taken early on 22 October (creatures stop fighting when their keepers
  become allies); the team fixed it on 31 October, and that fix reached only the alpha channel, with the
  2026-11-05 sync. Measured with a Bile Demon and a Skeleton in one lair: 1.4.0.6098 had seven fights, each
  over within half a second, with no damage; with this release, and with 1.4.0.5660 from before the change,
  both fight until the Skeleton dies. To keep such a pair apart, give each its own lair room: a fight starts
  only while one of them sleeps in its lair with the other in the same room.
- **Two ways a multiplayer game could fall out of step, closed.** When the Light spell ran out on a creature
  someone was possessing, the light was dimmed by whether the creature was hidden *on your screen*, which is
  different on each player's machine. And while a player possessed a chicken, the camera of that player's own
  game wrote the chicken's pecking angle into the game. Both now come from the game itself, the same for every
  player. The first-person camera can no longer change the creature or chicken it follows: the compiler now
  refuses it.

Saves are unchanged: same format as 1.4.0.6098. A replay recorded with 1.4.0.6062 to 1.4.0.6098 in which lair
enemies meet can play out differently from that point. Checked on this code, on hidden
displays with no sound: the lair fight above (two runs on the release build: both creatures fight until the
Skeleton dies, 14 and 10 seconds), every campaign's first level and all twenty of the original campaign on
OpenGL (35/35, the same engine messages as 1.4.0.6098), the OpenGL checks, the save, load and delete menus (the
same as 1.4.0.6098), and a two-player game by direct connection (19/19, no resync). The two desyncs need a
second player to possess a creature, which the test harness cannot script, so they were checked in the code:
the pecking angle the camera wrote is part of the game's multiplayer checksum, and the light now follows the
possession flag the game itself sets and clears.

## 1.4.0.6098 — 2026-11-06 — stable

**Replaces 1.4.0.6088.** A point release: 1.4.0.6088 with this edition's fixes since, and one of the KeeperFX
team's. The team's November changes (the 2026-11-05 sync) stay on the alpha channel until the next stable.

- **Our own online lobby.** Internet games are listed and joined through this edition's own lobby server,
  `lobby.keeperfx-tux.workers.dev`, and nothing goes to the KeeperFX team's servers any more. A `keeperfx.cfg`
  that still names the team's lobby is switched over by itself, by the game and by the launcher. The lobby only
  introduces the players to each other; the game runs directly between them, as before. Its list shows no
  addresses: a host's address goes only to the player who joins. Online play is with players on the same
  version of this edition. The team's Windows builds run a different simulation and could never join one.
- **Gold dropped into a temple's pool is no longer swallowed.** It vanished without a word. Now the mentor
  says *"This is no wishing well, Keeper."*, allies get the gold in multiplayer, and the temple counts it.
  This is the team's own fix from 6 November, taken early with its author. Measured with two builds identical
  but for the fix: the coins reach the pool in both, the message comes only with it.
- **For game managers such as Lutris** (issue #60): `KFX_NO_DESKTOP_ENTRY=1` stops the AppImage writing a menu
  entry and icon into the home folder. A menu entry you keep starts the game in the folder it was installed to
  (`KFX_ALPHA_PREFIX`), where it used to set up a second, empty copy in `~/.local/share/keeperfx-alpha`. And a
  newer AppImage keeps your `keeperfx.cfg`, adding any new settings, where it used to put every game setting
  back to its default.
- **The log is kept in a deep game folder.** With the game folder's path longer than about 130 characters
  (inside a Lutris or Flatpak data folder, say), the game ran without a log and without keeping the previous
  five. Measured at 177 and 297 characters.
- **Lua sandbox:** a script could get back the debug functions the sandbox closes, through the names the save
  serialiser keeps for them. Those names are gone. Maps still cannot load compiled code; this was the second
  line of defence.
- **More detail for an open bug:** when a worker turns up in a room that is not its own (*"Room ... is not
  valid ROOM_ROLE_..."*), the warning now says how it got there, to find the cause of the bursts seen on
  Burdened Imps.
- **Launcher:** crash reports come to this edition's issue tracker. The dialog saves a zip and opens a
  prefilled issue to attach it to, and sends nothing anywhere (it used to upload to keeperfx.net). The
  Multiplayer tab shows our lobby.

Saves are unchanged: same format as 1.4.0.6088, and saves go both ways (measured on Pandaemonium 4, whose
script keeps Lua functions: a 1.4.0.6088 save loads here and a save from here loads in 1.4.0.6088, with no
stubs and no Lua errors). Checked on this code, on hidden displays with no sound: two-player multiplayer by
direct connection (19/19, no resync), every campaign's first level and all twenty of the original campaign on OpenGL (35/35, the same engine messages
as 1.4.0.6088), the save, load and delete menus, the log in a 187-character folder and its rotation, the Lua
sandbox probes (all three routes to the debug functions refused), every campaign's first level under
AddressSanitizer and UndefinedBehaviorSanitizer (16/16, no memory errors, nothing in the changed code), the
temple's pool (the message and the count with the coin that carries the gold, nothing on the floor beside it),
and an online game between two copies on our lobby: both switched over from a `keeperfx.cfg` naming the team's
lobby, the second found the first in Online Lobbies and joined by hole punching, and a level ran in step on both.

## 1.4.0.6088 — 2026-11-05 — stable

**Replaces 1.4.0.6078.** The 1.4.0.6087 alpha, unchanged, as the stable release (details in its entry below):

- **The frame cap follows any monitor and no longer halves the frame rate by mistake.** It judges a frame by
  its own work, ignores the frames right after a load or a pause, steps down only when at least half the frames
  are late, and tries the faster rate again. On 1.4.0.6078 a player's afternoon on a 160 Hz monitor stepped
  down to 80 three times; on 1.4.0.6087 the same player's evening on the same level kept its cap throughout.
- **Variable refresh (FreeSync, G-Sync, VESA Adaptive-Sync).** The cap sits just under the top rate (157 on
  160 Hz) and never steps down. `VARIABLE_REFRESH=AUTO` (the default) asks the desktop; `ON`/`OFF` decide it,
  and the launcher's Graphics tab has the same choice. A `keeperfx.cfg` without the line counts as `AUTO`.
- **Full sharpness on a scaled desktop:** with the desktop scaled (measured at 125%) the game draws at the
  monitor's real resolution, instead of a smaller picture the desktop stretches.
- **Lua: `Map.default_texture`** keeps the texture a script set across a save and load.
- **Launcher:** the Variable refresh choice; the frame cap's fallback no longer stops at 200 Hz.

Saves are unchanged: same format as 1.4.0.6078 and 1.4.0.6072, and saves from both loaded in a player's
session on this code. Checked on 1.4.0.6087, beyond its entry: every campaign's first level under
AddressSanitizer and UndefinedBehaviorSanitizer (16/16, no memory errors, nothing in the changed code), the
AppImage on Ubuntu 24.04, Debian 13 and Fedora 41 (no missing libraries, the launcher starts, level 1 runs on
OpenGL), and an evening of play at 160 Hz with variable refresh detected (no crash, no new warnings).

## 1.4.0.6087 — 2026-11-04 — alpha

On top of stable 1.4.0.6078: frame pacing for any monitor, variable refresh, full sharpness on a scaled
desktop, and a Lua texture fix.

- **The frame cap follows any monitor and no longer halves the frame rate by mistake.**
  `FRAMES_PER_SECOND=AUTO` draws one frame per refresh of the monitor the game is on. Its step-down
  (144 → 72 when the machine cannot keep up) misjudged: it counted the time spent waiting for the monitor
  as work, judged the slow frames right after a load, stepped down when only 10% of frames were late, and
  could not climb back. 1.4.0.6072 halved 144 to 72 at the start of a camera sweep and stayed there; on
  OpenGL, where a frame takes about 1 ms, a player's evening still dropped to 72 three times after loads.
  Now a frame is judged by its own work, nothing right after a load or a pause counts, it steps down only
  when at least half the frames are late for three seconds, and it tries the faster rate again after 30 s
  (then 1, 2 and 4 minutes while that fails).
- **Variable refresh (FreeSync, G-Sync, VESA Adaptive-Sync).** On such a monitor a late frame is shown as
  soon as it is ready, so the cap never steps down and sits a little under the top rate to stay in the
  variable range: 141 on 144 Hz, 157 on 160 Hz. `VARIABLE_REFRESH=AUTO` (the default) asks the desktop —
  GNOME says — and `ON`/`OFF` decide it; the launcher's Graphics tab has the same choice. Measured on a copy
  of a player's save at 160 Hz with variable refresh: 157 frames per second throughout, 0.65 ms a frame.
- **Full sharpness on a scaled desktop.** With the desktop at 125% the game took 3440×1440 as 2752×1152 and
  the desktop stretched the picture back up. It now draws at the monitor's real resolution (measured: 2752×1152
  before, 3440×1440 now, a third more edge detail), windowed modes are the stated number of pixels, and the
  mouse moves exactly as before.
- **Lua: `Map.default_texture`.** A texture a level's script set read back one off and turned into the next
  texture after a saved game was loaded. The team fixed the reading half (their #5583, taken here with its
  author); this fixes the half that stores it. Measured: set "WINTER", save, load — still winter (on 1.4.0.6078
  the reloaded terrain changed).
- **Launcher:** the Variable refresh choice on the Graphics tab; the refresh rate it writes as the frame cap's
  fallback no longer stops at 200 Hz (a 240 or 360 Hz monitor is not capped by it).

Saves are unchanged (same format as 1.4.0.6078). Checked on this build, on OpenGL: every campaign's first level
(35/35, the same engine messages as the stable), save and load, the save menu, two-player multiplayer for three
minutes without a resync, resolution changes in game, and copies of a player's saves through a level's final
fight and on the next level.

## 1.4.0.6078 — 2026-11-04 — stable

**Replaces 1.4.0.6072.** The graphics card now draws the game, and every machine still starts it:

- **OpenGL is the default renderer.** In busy parts of a map — fights, spells, lots of effects — the software
  renderer needs more time per frame than a fast monitor gives it, so frames are held and the view judders even
  though the game itself runs at full speed (the slowness reported on 1.4.0.6072). Measured with the camera
  sweeping a copy of a Burdened Imps save, no frame cap, median frames per second:

  | screen | software | OpenGL |
  |---|---|---|
  | 1920×1080 | 311 | 520 |
  | 2560×1440 | 175 | 454 |
  | 3440×1440 | 129 | 488 |
  | 3840×2160 | 83 | 446 |

  On OpenGL at most 0.6% of frames take longer than even a 360 Hz monitor allows; on software at 3440×1440, 58%
  take longer than a 144 Hz one does.
- **The software renderer takes over wherever OpenGL cannot start** — an old graphics card, a virtual machine,
  a broken driver — and `keeperfx.log` says why. Before, choosing OpenGL on such a machine meant the game did
  not start.
- **Movies play in full colour on OpenGL.** It used to show them in 256 dithered colours (about 40 distinct
  colours in a frame of the intro, against 32,000–115,000 now, the same as the software renderer). All five
  shipped movies checked on both renderers.
- **Launcher: existing installs switch once.** A `keeperfx.cfg` from before this release says
  `RENDERER=SOFTWARE` because that was the shipped default, so the launcher moves it to OpenGL the first time it
  sees this engine, and notes that it did. If you then choose Software again (Settings → Graphics → Renderer),
  that choice is kept.

Saves are unchanged (same format as 1.4.0.6072). Checked on this build, on OpenGL: every campaign's first level
(35/35), save and load, the save menu, two-player multiplayer for three minutes without a resync, three
resolution changes in game, and a copy of a player's save through a level's final fight.

## 1.4.0.6072 — 2026-11-03 — stable

**Replaces 1.4.0.6062.** A security fix for downloaded maps, mods and campaigns, Workshop maps on the AUR
package, and two launcher fixes:

- **A map's Lua can no longer reach outside the game.** Level, campaign and mod scripts arrive with
  downloaded content, and three routes led past the sandbox:
  - *FFI.* The engine never opens LuaJIT's FFI library, but LuaJIT loads it by itself the first time it
    reads a 64-bit number such as `1LL`, so two lines of a map's script could call any C function on the
    machine (the team's issue #5567). It is now loaded once when a level starts and taken away; `1LL`
    keeps working.
  - *Bytecode.* LuaJIT runs precompiled Lua without checking it, so a crafted chunk can read and write
    any memory. It came in through `loadstring`/`load`, through `loadfile`/`dofile`/`require` and a map's
    own `.lua` file (which could be precompiled), and through saved games: the game stored the functions in
    a level's script state as bytecode and loaded them back from the save file, from the state a host sends
    in multiplayer, and from whatever a map had put in that state. Every one of those now accepts source
    text only.
  - *The debug library*, which reaches everything the game's own Lua holds. Scripts loaded after the
    game's library keep `debug.traceback` and `debug.getinfo`, the parts the shipped campaigns use.
  Measured with harmless payloads that announce themselves if they run: on 1.4.0.6062 eight of them ran
  and FFI called a native function; on this build none did.
- **Saved functions are kept as their source text.** So a save still restores the script functions it
  holds — trigger callbacks and the like — and now even after the script that defined them is edited
  (upstream's code read the script again at load time and fell back to the bytecode when it could not).
  Saves made by 1.4.0.6062 and earlier load: their functions are rebuilt from the scripts, matched against
  the stored bytecode as bytes, never run. Checked on Pandaemonium, Heroes' Journey, Sapphire Realms and
  the original campaign: every function in a 1.4.0.6062 save came back (48, 27, 43, 71 and 11 of them on
  the levels that keep any), none was lost, and no level logged a Lua error.
- **AUR: Workshop maps install.** On the package install (Arch, Omarchy, EndeavourOS, CachyOS, Manjaro:
  any pacman system) the launcher refused most Workshop maps
  with "Could not write into levels": a map adds itself to the built-in Standard levels pack, and that
  folder was a link to the package's copy under `/usr`, which only root can write to. The same went for
  the other built-in map packs and campaigns and for `multiplayer`. Every folder in the game folder is
  now your own, and only the game's own files inside are links to the package, so upgrades still
  arrive straight from pacman. The first start after the upgrade takes a few seconds longer while it
  sets this up, and a folder you made read-only no longer stops the game from starting.
- **Launcher: installed Workshop maps show as installed.** A Workshop map that adds itself to the
  built-in Standard levels pack (the way most single maps ship) never appeared in the Workshop's
  *Installed* tab, so it could not be uninstalled on its own, and *Browse* kept offering *Install*
  after it was installed. The launcher now keeps a record of what each install adds, so the map is
  listed under its name, uninstalls and restores by itself, and *Browse* marks it *Installed*. The
  built-in maps beside it are never touched. A map an earlier launcher installed shows up after one
  *Reinstall*.
- **Launcher: no older alpha offered on the alpha channel.** A launcher set to the alpha channel offered
  1.4.0.6013-alpha to a 1.4.0.6062 install — a downgrade — because the team's rule offers the other
  channel's release whenever the channel differs. An alpha is now offered only when it is newer;
  switching back to stable is still offered either way.

Saved games from 1.4.0.6062 load unchanged. Going back is one-way for some saves: a save this version makes
on a level whose script keeps functions (Pandaemonium and Heroes' Journey levels among them) does not
load in 1.4.0.6062, which cannot read functions kept as text; saves on other levels, the original
campaign's included, do.

## 1.4.0.6062 — 2026-11-03 — stable

**Replaces 1.4.0.5660 as the stable release.** Everything the alpha line accumulated from 1.4.0.5696
to 1.4.0.6013 (below), plus the fixes made since 6013 (listed here), promoted after a full pass:
static analysis of the engine with every finding checked against the code, valgrind on 35 levels for
15 minutes each, every campaign under AddressSanitizer and UndefinedBehaviorSanitizer, all 35 level
starts, six replays played back in step, a ten-minute multiplayer game, save and load on eight levels
and on copies of players' saves, the AppImage started on Ubuntu 24.04, Debian 13 and Fedora 41, the
Arch packages upgraded from the real 5660 ones, and many hours of play on the alpha line.

> **Saved games from 1.4.0.5660 and earlier cannot be loaded by this release — for the last time.**
> From this release on a saved game describes its own layout and is read field by field, so saves made
> with it keep loading in later versions. Your 5660 saves stay on disk and are listed greyed out under
> their titles; they still open in 5660, which updating keeps (`keeperfx-tux-previous` on Arch,
> `previous/` beside the game for the AppImage and Flatpak). Finish a level in progress on 5660 first,
> or replay it from its start. **Campaign progress — Continue, completed levels, bonus levels and the
> creatures carried to the next level — carries over, and so do high scores.** The release notes
> repeat this automatically.

In short, since 5660:

- **Two months of the team's work.** Five syncs through 19 October and fixes from the week after:
  their OpenGL renderer (opt-in with `RENDERER=OPENGL`; the software renderer stays the default), the
  new save format, a progress file per campaign, a replay recorded for every campaign level (zoom,
  pause, hold Backspace to rewind), claimed rooms that merge with their neighbours, multiplayer
  spectators and colours picked in the lobby, desync fixes, and much else.
- **No Windows in this repository.** The team's code is merged with its Windows-only parts removed,
  proven not to change what Linux compiles.
- **Bugs from players' games.** Workers quitting a room they cross, scavengers taken out of their room
  every 20 turns, imps fetching crates for traps they cannot reach, hatchery counts, rebirth, magic
  doors, explosions, abyss kills, script commands that missed the creatures already on the map.
- **Security.** Chat and replays can no longer act on your machine, and Lua's own file functions no
  longer let a map script write or delete your files.
- **64-bit fixes.** Creature shadows in the software renderer, level-script effects, timers, movie
  recording, high-score files, the light table in saved games, the hand's config flags.
- **Launcher.** Renderer, map fade, viewport mode, zoom distance and replay settings; a golden keeper
  for stable releases in the news; Save no longer switches settings off on an older config (below).

**Since 1.4.0.6013-alpha:**

- **The game no longer closes after a few Lua errors.** When a Lua command failed — typed in the
  console, or a map script's `RUN_LUA_CODE` — the engine took two values off Lua's stack where a failed
  command leaves one. Each failure sank the stack a slot, and on the sixth LuaJIT exited: the game
  closed with nothing in the log but "Application exit called", and nothing was saved. A mod's Lua hook
  (creature states; object, trap, door and shot updates; trap activation; shot hits) that raised an
  error did the same every turn, and took its result from below the stack (for an object's update, a
  negative value means "deleted"). Measured: 40 failing commands and 40 that return values, and an
  object whose update always fails for 20 seconds, with the game and Lua still running. Upstream has
  the same bugs.
- **Saved games on levels with their own config files read those files again** — the team's fix
  (#5550), taken ahead of the next sync. A save stored the "selected level" as 0, so loading one read
  `map00000.*` instead of the level's own rules, objects, slabs or creatures. On Pandaemonium level 4
  the reload fired the level's final assault at once (hero creatures on the map 61 → 77). Heroes'
  Journey, Pandaemonium, Sapphire Realms and some personal levels ship such files.
- **Creatures no longer choose a step from leftover memory.** 52 height lookups built their position
  from x and y only, and so did a temple sacrifice drop, a room spot for a new imp and a wall-hug step:
  whatever the stack held decided the outcome, so replays and multiplayer peers drifted apart (a level
  220 replay went out of sync at turn 6,200). valgrind on that replay: 36 uninitialised-value errors
  before, none now. Replay files no longer carry unset bytes in their header either. A replay recorded
  by an earlier version can go out of sync part-way through.
- **A level saved part-way through replays in step.** The save put every replay of that level a turn
  behind from that point ("Out of sync" on every turn, every later move a turn early), and rewinding
  (hold Backspace) across any pause dropped you to the main menu. The fix is in the reader, so earlier
  recordings with a save in them replay correctly too.
- **Map-script fills over a large area no longer write past their buffer.** `CHANGE_SLAB_OWNER`,
  `CHANGE_SLAB_TYPE`, `CHANGE_SLAB_TEXTURE` and reveal fills kept a 16-bit entry per slab in room for
  one byte per slab. No shipped level comes close (the largest of the 111 shipped fills needs 52).
- **Mods that hang the game or reach past a buffer.** A mod defining a 256th room type froze the game
  at load; the count is now capped at 255 with a warning, as `NEW_ROOM_TYPE` already does. A dig shot
  with a modded Health of 256 or more could hang first-person digging. Room teardown on a corrupted
  slab list, two route lookups, a negative effect id and a path helper no longer write or read past
  their buffers.
- **Map-script settings that did nothing.** `SET_POWER_CONFIGURATION` set 0 for Spell, Effect,
  UseFunction, CreatureType and CostFormula (and Spell wrote the cooldown); a power's top overcharge
  level charged its duration as the price; a matching `BLOCKED_PICKUP` hand rule blocked instead of
  lifting the block; party range checks could never be true.
- **No memory leak on every level start.** The TOML parser that reads the slab, column, effect and
  level files kept every table name: 92–124 KB per level start. The sanitizer pass now checks that
  parser too. A campaign without a credits line (revlord) no longer opens a folder as its credits.
- **The hand keeps its object flags after a one-flag Castability change.** On 64-bit Linux, config
  fields of type `long` were read back as 32 bits, so `Castability = <FLAG> 1` in a mod, a campaign's
  `magic.cfg` or a map script wiped flags 32–40: picking up and slapping objects. Stable 5660 has the
  same bug. Also the team's fix for the minimap zooming fully in when a save loads (#5540), taken early.
- **The game keeps its last five logs** (`keeperfx.log.1` to `.5`); every start used to overwrite the
  log of the run before.
- **Found by the sanitizers:** a multiplayer host read before the start of the session list on every
  lobby update, and a game folder whose log path came to exactly 144 characters wrote a byte past a
  buffer.
- **The startup error dialog shows without libXrandr.** On a system without it (a minimal Fedora, for
  one) the AppImage's engine segfaulted instead of saying why it could not start — the known issue in
  the 1.4.0.5652 notes. The AppImage now carries libXrandr.
- **Arch: the package replaces copies an AppImage left in the game folder.** They shadowed every
  package upgrade (August artwork, no scrollbar on the Load menu). Identical copies become links;
  anything else is moved to `.replaced-by-package/`, never deleted. A weekly check now flags an Arch
  library update that would need the engine rebuilt.
- **Launcher: Save keeps the game's defaults.** A `keeperfx.cfg` older than a setting has no line for
  it, and the game uses its default; the Settings dialog read the missing line as off or empty, and
  Save wrote that back: VSync, relative mouse mode (the cursor jumped on Wayland), delta time, edge
  panning and the possession cursor lock off, the startup screens and the zoom limit gone, and the
  creature flower, select box, hand sizes and API port saved empty, which the game reads as 0. A config
  already saved that way keeps its values: tick **Relative mouse mode** (Input) and **Enable VSync**
  (Graphics) once.
- **Launcher:** stable releases in the news panel carry a golden keeper; alphas keep the red one.
- **Quieter logs:** a boulder sinking into lava, deleting a switched-off light and moving crates off a
  lost workshop slab no longer log errors.
- Downloads are named `KeeperFX-Tux-Edition-x86_64.AppImage` and `.flatpak`; the AppImage is attached
  under its old name too.

Known:
- With `RENDERER=OPENGL`, movies play in 256 colours; the software renderer plays them in full colour.
- With *Automatically determine max. FPS* on, the frame cap can step down after a level loads (to
  72, 48 or 36 fps on a 144 Hz display) and take a while to come back. A fixed rate is never adjusted:
  untick it on the Graphics tab and set Max. FPS to your display's rate, or use the OpenGL renderer.

## 1.4.0.6013 — 2026-10-23 — alpha

- **Movies play in full colour again.** Every movie the game ships — the intro, the logo stings, the
  torture-room and outro films — is modern H.264 video. Up to stable 1.4.0.5660 they played in full
  colour and were scaled smoothly to the screen. When the team's new renderer arrived with the
  16 September sync, the fork's display layer that did this went with it, and from 1.4.0.5698-alpha
  on every frame was squeezed into 216 fixed colours with a dither pattern, then enlarged with
  uneven pixels: speckled dark scenes, banded light, a cross-hatch over everything on a big screen.
  Found by a player on the first evening of 1.4.0.6010. The team's renderer now takes a full-colour
  image too, and the software renderer draws movie frames on a texture of their own, smoothly
  scaled. Measured on the same intro frames at 3440x1440: 30–43 distinct colours in the middle of
  the picture in 1.4.0.6010, 27,000–61,000 now (stable 1.4.0.5660: 31,000–82,000), and side by side
  the frames match 5660's. With `RENDERER=OPENGL` movies still play in 256 colours for now; the log
  says so once.
- Same save format as 1.4.0.6010: its saves load here. Coming from an earlier version, read the
  1.4.0.6010 notes below first.

## 1.4.0.6010 — 2026-10-22 — alpha

> **Saved games from earlier versions cannot be loaded by this build — for the last time.** With the
> team's new save format (below) a saved game describes its own layout and is read field by field, so
> saves made with this build keep loading in later ones. Saves from 1.4.0.5762-alpha, from stable
> 1.4.0.5660 and from every earlier build are kept on disk and listed greyed out under their titles;
> they still open in the version that wrote them, and saving into one of their slots moves the old
> save to the trash instead of writing over it. Finish a level in progress on your current version
> first, or replay it from its start. The release notes repeat this automatically. **Campaign
> progress — Continue, completed levels, bonus levels and the creatures carried to the next level —
> carries over, and so do high scores** (see below).

**No more Windows in this repository.**
- **The team's Windows content is gone, and the weekly sync cannot bring it back.** Their Visual Studio
  project, the MinGW/MSVC CMake build and vcpkg, a Windows `gdb.exe`, CUnit's Visual C++ solutions,
  the Windows-only platform code, their Windows CI, and every `#ifdef _WIN32` branch in the engine and
  the vendored libraries — 281 files, about 101,000 lines. The sync no longer merges the team's
  commits directly: it takes their tree, removes the Windows content (`packaging/linux-only/`), and
  merges that snapshot, so Windows files are on neither side of a merge and never conflict again.
  **Proven not to change the game:** every file the Linux build compiles was preprocessed with and
  without the filter — 315 identical, one with identical machine code — and the sync repeats that
  proof every week. A guard workflow fails any pull request that reintroduces Windows content.
- The sync also adds the team's new source files to `linux.mk` by itself (previously a manual edit
  after every sync that added one), generates the fonts and runs the unit tests, and when it cannot
  finish it updates the open sync issue instead of opening a new one every Monday. Upstream's
  `Makefile` stays: the release builds use it to regenerate the artwork.

**The team's changes through 9 October** (48 commits, merged through the new Linux-only sync).
Among them: every campaign you play keeps its own progress, and a saved game that fails to load
resumes that campaign instead; replays can be zoomed, paused, turned off, and survive resyncs;
multiplayer gains spectators, keeps lobbies listed while playing, and fixes several desyncs; the
parchment-map fade is on by default at every resolution (`PARCHMENT_MAP_FADE=OFF` turns it off);
creatures and objects get configurable inertia and fall acceleration; Lua scripts are sandboxed and
read the date and time without desyncing multiplayer; plus fixes to flying over lava, boulders,
hatcheries, the abyss, possession and camera stutter with VSync. Two of these needed Linux fixes:
- **Campaign progress carries over.** The team moved progress from one `fx1contn.sav` to a file per
  campaign and imports the old file once — but only in the layout 32-bit Windows writes. Every Linux
  build wrote it 160 bytes longer (its campaign flags were 8-byte `long`s), so the import would have
  skipped it and the Continue button would have vanished with every player's campaign position. The
  64-bit layout is now imported too, checked field by field against a file written by stable
  1.4.0.5660 itself. The old file is also left in place instead of deleted, so going back to a
  stable build that predates the change keeps that build's own *Continue*.
- The new multiplayer matchmaking passed curl a 4-byte timeout where curl reads 8 bytes on 64-bit
  Linux, leaving half of it undefined.

**The team's changes through 19 October** (48 commits, 13–19 October). The big one is the **new save
format**: saved games, the continue link and campaign progress describe their own layout and are read
field by field, so a change to the engine no longer makes every older save unreadable — after this
one last time (the box above). Among the rest: hold Backspace to rewind a replay, and replays skip the
intro cinematic; claimed rooms merge with adjacent rooms of the same kind; in multiplayer you pick your
colour in the lobby, lobby buttons respond at once, disconnect messages are clearer, and spectators are
shared among the players, switch view with Space or a click on a name, and can sync again; a faster
software parchment-map fade; a destroyed heart reveals the map after its explosion, not before;
tooltips no longer hide behind the hand; possession no longer slides or turns the creature north, and
the hand's light is off while possessing; chickens no longer breathe fire; the computer no longer
reuses, as defenders, creatures it has just dropped; Shift+M recording shows a timer;
`CHAT_FONT_SIZE` in `keeperfx.cfg`; `USE_SPECIAL_STEAL_HERO` and alternate player names in script
locations. It also brings fixes of theirs this fork had already taken early, among them a
temple that counted a creature leaving it twice, and so shielded fewer creatures from enemy scavenging
than were praying in it, and drag and shaped room tools that could start one slab outside the shape.
Some of it needed work here:
- **The hand can pick creatures up.** The team's new config check limits `long` settings to 32 bits, as
  on Windows. On 64-bit Linux the hand's, Slap's and Pick Up Object's "what can it touch" flags are such a
  setting and go past 32 bits, so they were refused at every level start and the hand picked up
  nothing. Those settings take their full 64-bit values again, as in every earlier Linux build.
- **Campaign progress still carries over.** The new code again imports the old `fx1contn.sav` only in
  the 32-bit layout (67,241 bytes). Every Linux build wrote 67,401, stable 1.4.0.5660 included, so a
  5660 player would have lost Continue and the progress percentage, and the campaign, its bonus
  levels, carried creatures and flags would have started over. The 64-bit import is back, the
  old file is still kept for stable, and Continue falls back to it only while that campaign has no
  progress file of its own.
- **Saves from earlier Linux builds are listed under their titles**, not as `fx1g0003.sav`. The new
  code reads a save's title after a 12-byte header; every save 64-bit Linux builds wrote before
  1.4.0.5751, stable 1.4.0.5660 included, has 24-byte ones. Checked against a save stable 5660 wrote.
- **The message for a save that cannot be loaded is not blank.** The download's game text came from
  the team's July release, so every message added since showed as nothing: that one, and the newer
  spectator and lobby messages. Every release now compiles the text from the team's translations, as
  it already regenerates the artwork.
- **A save in a deep folder loads.** The new loader cut a save's path to 144 bytes, so with a long
  home directory or a Flatpak data directory the file read as missing and its slot was disabled.
- **A merged room keeps the creatures heading for it.** Creatures on their way to a room that merges
  into its neighbour, or that last worked there, still pointed at the absorbed room, whose number then
  goes to the next room built.

**Ten of the team's fixes from the week after, taken early:** creatures stop fighting when their
keepers ally; creatures no longer ask for a lair while you cannot build one; moving a library's books
no longer turns that spell off; navigation is updated when a neighbouring slab changes; possession
camera fixes; rewinding a replay restores the maps and event boxes; no crash when the host leaves after
a failed join; a possible wrong slab display; a creature's previous work room survives changes to its
current one; and the hand's pickup priority. Next to the books fix, ours: another keeper losing a spell
— claiming or robbing an enemy library, or a neutral one shrinking — no longer clears your own
selection of it.

**Creatures, rooms and imps.** Most of these were found in players' logs from stable 1.4.0.5660.
- **Workers keep working in their own room.** A worker crossing a slab of another room between spots —
  a bridge through a training room, the training room beside the workshop — quit: about a hundred
  "is not valid ROOM_ROLE_TRAIN_EXP" lines in one log, most of them imps dropped in to train. On copies
  of that player's saves: 9 and 37 such quits on 5660, none with the fix; in a U-shaped training room
  around a bridge, all 8 trainees gone within 30 seconds before, all 8 reaching level 2 after. A worker
  stepping off its room's edge for a moment no longer pays the training fee for no experience.
- **A creature dropped into training once and scavenging later** was taken out of its scavenger room
  every 20 turns and enrolled again, for the rest of the game: a computer keeper's vampire on original
  level 20 did it 382 times in one session.
- **The computer's creatures the training room cannot take** (level 10, or training it cannot afford)
  go to the room they were picked for, not onto its heart: 355 errors in one level 20 log; on a staged
  level 20, 69 in about 4000 turns before, none after.
- **Imps and traps.** An imp that finds no route on to an empty trap passes over it for 1000 turns
  instead of fetching a crate for it about every 130 turns (one log: a single trap failing 63 times over
  17,000 turns; staged, 37 failed deliveries in about 5000 turns before, 5 after); arming the trap
  clears the pause, and saved games keep it. An imp whose trap is sold or destroyed on the way no
  longer takes whatever is given the trap's place for it (a possessed imp would have "re-armed" it).
  The workshop tab's count of placeable traps is worked out from the crates and traps themselves,
  instead of a running tally that crept up and offered traps there were no crates for.
- **Hatcheries.** Shrinking a full hatchery left more chickens than it holds while counting fewer (8 in
  a room for 5, then 7 counted as 1 of 2); the count now matches the room. A hungry creature whose
  chicken is eaten on the way chooses another, instead of leaving hungry or eating whatever took the
  chicken's place.
- **A creature reborn at its lair starts afresh**, instead of carrying on in the state it died in (a
  vampire killed at a door came back still "in door combat").
- **Fights.** A magic door that holds a boulder uses it up instead of leaving it 1–7 health; an
  explosion or poison cloud hits everything on a subtile even when one hit removes another thing; a
  creature pushed into the abyss credits its pusher with the kill again (only a fall ending within 15
  turns of the push counted, and the fall takes at least 28). **Call to Arms** no longer sweeps a
  black strip across the view after each cast: its charge bar ran past full, since the spell's
  duration is 1.
- A creature going for its pay no longer chooses its route from leftover memory, which can differ
  between machines (a possible multiplayer or replay divergence). A script objective at a destroyed
  dungeon heart points where it stood.

**Multiplayer and replays.**
- **Chat and replays can no longer act on your machine.** A command typed in chat runs on every
  machine that sees it, replays included, and none checked who typed it: a replay containing
  `!game.save 1` overwrote the viewer's own save, a recorded `!game.load` could crash the viewer, and
  one player's `!game.save N` overwrote slot N on every machine in the game. Commands acting on this
  machine — its files, its settings, quitting — now run only when typed on it, never for another
  player's chat or a replay's. Gameplay commands replay as before.
- **A chat message must come from the player it names.** Any peer could speak as another: issue
  console commands as the host's player (which pass the cheat check when the host allows cheats), or
  start the game from the lobby as if the host had chosen a level.
- A damaged or hostile replay file is refused instead of crashing the game or reading past a buffer.
  Old replays are trimmed however long the install path (from about 125 characters nothing was
  removed); a size limit of 4 GB or more no longer stops recording at once; a resync read as you stop
  a replay is no longer applied to the next one.

**64-bit fixes.** KeeperFX was built for 32-bit Windows, where a `long` is 4 bytes; on 64-bit Linux
it is 8, and these behaved differently from Windows:
- **Creature shadows and other textured polygons in the software renderer** (the default) sampled the
  wrong texture rows: the rasteriser's carry helpers worked on 64-bit values. Checked against a
  32-bit model on 54,000 random triangles in all 27 drawing modes: 14,943 differences before, none after.
- **Level scripts:** `ADD_EFFECT_GENERATOR_TO_LEVEL` placed nothing, `CREATE_EFFECTS_LINE` aimed at a
  garbage location, `DISPLAY_VARIABLE_WITH_LABEL` read past the end of an array — script arguments
  packed for 4-byte slots overlapped in 8-byte ones. The slots are 32-bit now, as on Windows, which
  also retires the fork's earlier per-command workarounds.
- **Script timers and bonus time go negative again** after `ADD_TO_TIMER` with a negative value, instead
  of reading about 4.29 billion and firing `IF(...TIMERn >= X)` at once. **Sight of Evil** recast in the
  first minute of a level moves the sight instead of closing it. Lua's `unsummon_duration` reads 0 for a
  creature that is not summoned.
- **Movie recording (Shift+M)** wrote files nothing could open and crashed on the second frame.
- **High-score files** in the team's new format and in Windows' 72-byte layout are read as well as
  the 80-byte one Linux builds write. Saving keeps the 80-byte layout for now, because stable
  1.4.0.5660 shares the game folder and reads only that one: given anything else it would replace the
  whole table with Bullfrog's defaults the next time a player switched back.
- **The light table in saved games** was twice its saved size: saving copied 256 bytes into a 128-byte
  field, over the light counters and 100 bytes of game state after them, in the running game and in
  the file, and loading put dynamic lights in the wrong shadow-cache columns.

**Mods and maps made on Windows.** Windows does not care about upper/lower case in file names or which
slash is used; Linux does. Now handled the Windows way:
- custom sounds, a level's or campaign's Lua scripts (and `require`), map and sprite zips, and map-pack
  folders (`LEVELS_LOCATION = levels/MyPack` beside a real `levels/mypack`) are found whatever the case;
- `\` in a path (`creature\maiden\hit1.wav`, `campgns\mycamp`) is read as a separator;
- a folder name containing `[`, `?` or `*` (an install at `/games/DK [GOG]`) is no longer a wildcard;
- Korean text is decoded as code page 949, as on Windows (EUC-KR lacks most Hangul), and one bad
  character no longer empties a whole text file;
- `MUSIC_FROM_DISK=OFF` or `-cd` play the music folder (there is no CD audio on Linux) instead of nothing.

**A downloaded map's scripts can no longer change your files.** Level and campaign Lua comes with
maps and Workshop items, and the team's new sandbox still let a script overwrite or delete any file
you own, or load a native library shipped beside it. Scripts can now read files and nothing more:
writing, appending and deleting are refused the way a failed open always was (so a script that checks
carries on), and `require` no longer loads native code. Nothing the game ships or the team's tree
contains uses any of that; the game's own Lua, including how it saves and restores script state, is
unaffected. It closes the one-line attacks, not every route: the full write-up is in
`src/lua_readonly_io.c`.

**Map scripts.**
- Lua scripts keep `os.date`, `os.time`, `os.clock` and `os.difftime`, which the team's sandbox had
  removed: the Pandaemonium campaign's second level builds its welcome message with `os.date`, so its
  setup stopped there — no start money, creature limit, rooms, traps, allies or triggers. Multiplayer
  still replaces them with its "not synchronised" error.
- `SET_CREATURE_CONFIGURATION`: `INERTIA` and `FALLACCELERATION` reach the creatures already on the map
  (a map places its creatures before its script runs, so they never did), and `HEALTH` and `BASESPEED`
  reach neutral ones; leaving out `INERTIA`'s air value or the height of `SIZE` and `THINGSIZE` keeps
  the current value instead of setting 0; a missing `LairEnemy` is no longer stack garbage (a random
  lair enemy, and a possible multiplayer divergence). A creature rescued from the abyss keeps its own
  fall acceleration.
- `MOVE_CREATURE` for `ALL_PLAYERS` no longer spawns a stray effect at both ends of the move; removing
  a sacrifice recipe from a full table no longer leaves the last one in two slots; `DESCRIPTION_GEO`
  boxes on a campaign's land view are drawn where they are told, and a level without a description no
  longer shows the previous one's text.
- A box tooltip (`SET_BOX_TOOLTIP` or Lua) containing `%s` no longer crashes the game when a possessed
  creature picks the box up: map and config text is never used as a format any more. A long
  `SACRIFICED[...]` name from Lua or the API no longer overflows the stack.

**Saved games.**
- A damaged save (truncated by a full disk, a block of the wrong size) is refused before anything is
  loaded, instead of half-loading into the running game; the team's new save code now checks and
  decodes a save before it touches the game. A refused load from the in-game menu restores the pause
  state it found.
- Esc closes the delete-save confirmation instead of leaving it over the game with "Yes" still armed.
- A save named in capitals (`FX1G0003.SAV`, from a DOS or GOG install) can be deleted; the trash never
  overwrites a file already in it, and an incomplete copy never costs the original.

**Smaller fixes.** In a music folder that mixes `keeperNN` files with other songs, every song plays once
(one repeated and another never played). A mod's misspelt icon shows the checkerboard placeholder again
rather than an empty button. A movie in a pixel format the converter cannot handle is skipped instead
of crashing. The game's API server no longer exits silently when a client disconnects (SIGPIPE).
`TRASH_MAX_COUNT`/`TRASH_MAX_DAYS` moved out of the spots upstream edits every sync, which removes one
of the files that conflicted in five of the last seven syncs. With the OpenGL renderer, a level load
finishing at the wrong moment no longer leaves the old or a mixed tileset. The API's `get_level_info`
no longer crashes the game for a level outside the campaign. Undefined behaviour is gone from reading
the FPS setting, from the creature job tables (jobs 33 and up aliased jobs 1 and up) and from a
navigation mesh with a missing link.

**Quieter logs.** These are ordinary play and no longer warnings: an imp finding a box it may pick up
(59 lines in one level 20 log), selling nothing, placing a trap with none left, an original-format level
without a `.lgtfx` file, a hurt imp with nowhere to flee resting where it stands, a destroyed heart's
last update, a slapped or spell-casting creature not yet free to fight, an imp's backwards move whose
route closes on the way, a player who has already lost, and the doors of a level started after another
(whose navigation was also rebuilt for nothing). A room with too many things to put back warns when one
is lost, instead of losing it silently; the out-of-range creature-state warning names the states around
it; the hatchery-capacity and door-combat messages print their values in order.

**Packaging.**
- **AUR:** what is published is exactly what the checksums were computed from (a script edited after
  tagging used to ship with a stale hash, failing `makepkg` for everyone); a version pacman would never
  offer — lower than, or equal to but different from, the live one — now fails loudly instead of
  "publishing" to nobody. The launcher's self-update is no longer undone on every start. On a fresh
  install, opening the launcher first (the "KeeperFX" menu entry) now sets up the game directory, so it
  offers to copy in your Dungeon Keeper files instead of offering to download all of KeeperFX again.
  Publishing now waits until the AUR actually lists the new version (it can lag its own git repository
  by minutes, so a green run used to be followed by `yay -Syu` finding nothing), and records the version
  bump on the branch the release came from rather than always on `alpha`.
- **AppImage:** an unbundled library now fails the build (the check only warned); the current
  directory is no longer on the library search path.
- `linux.mk` checks the prebuilt dependencies against pinned SHA-256s and never caches a failed
  download. The fonts are generated by one script instead of five copies. The monthly Flatpak finds
  its stable release past 20 alphas.
- The release guard checks the save format's field tables with the team's own tool and policy check,
  instead of the size of the game state: a field missing from the tables would never be saved, and
  nothing else would notice. The team's save tests run with ours. A failing step in the sync or
  sanitizer builds no longer passes as green, and re-running an AUR publish whose only change is the
  launcher pin no longer fails.

## 1.4.0.5762 — 2026-09-28 — alpha

- **The High Score Table names the level and titles its columns.** Bullfrog's table printed a level
  *number*, which reads fine in the numbered original campaign and meant nothing for a custom map (a
  personal map showed as "30117"). Each row now shows the level's name, looked up in the campaign
  or map pack the table belongs to, under a Score / Level / Name title row; the number is kept as
  a fallback for a row the selected pack has no such level for, because several packs write the
  same score file. Long names are cut at the column edge; nine rows are visible instead of ten.
  The score itself is untouched: Bullfrog's peak dungeon rating, doubled for free-play maps.
- **The artwork now tracks upstream, not their July release.** The payload took its sprite sheets,
  fonts, textures and land views from upstream's last *stable* package while the engine in the
  same download was built from their *master*, so anything upstream added together with new art
  shipped here without the art. Found with the in-game Load/Save scrollbar (their #5106): six new
  sprites, a sheet with 239 entries against 245 wanted, "Reserved button sprite 239 has no
  matching entry" in every log, and no scrollbar at all for anyone with more than eight saves.
  Fourteen upstream artwork commits since July were missing by now. Every release build now
  regenerates the artwork from upstream's FXGraphics repository the way their own builds do, so
  this download carries the scrollbar, the coloured message icons, the multiplayer bullfrog
  colours, the font fixes, the torture screen and the floor next to the Abyss slab.
- **When an imp cannot deliver a crate to a trap, the log now says why.** "Cannot deliver crate"
  appeared two dozen times in one session for traps on plain claimed floor, and every reason the
  engine can refuse the walk was logged only at debug level. The failure now writes one line with
  the pieces that decide it: the in-wall check at the target, the route with and without the
  locked-door rule, the mesh regions at both ends and whether they connect, the owner's door
  counts and the map-changed flag. Only on failure, so it costs nothing in normal play.
- **README:** before/after pictures of the high score table on a custom campaign and a custom map
  pack, a three-step picture of deleting a saved game, and a live "this page last updated" badge.
- **Launcher:** the **Viewport mode** dropdown, the **Max. zoom distance** slider and the replay
  controls (**Max. replay size**, **Keep replays** per campaign / free play / multiplayer), written
  under whichever key names the running engine reads and showing the engine's defaults when a
  config has no replay lines; the retired *Command character* field is gone; the log viewer's open
  folder button and the workshop links open with the AppImage's library path stripped.

- **Saved games from 1.4.0.5732 keep loading.** The save format did not move this time
  (`sizeof(struct Game)` is still 53383086 bytes), measured by the release guard, not assumed.
- **Upstream sync: dkfans master through d19666f9a (14 commits, 23–29 September).** Every game is
  now recorded as a compressed replay under `replays/` (campaign, free-play and multiplayer folders),
  kept to `MAX_REPLAYS=5 5 10` files of at most `REPLAY_MAX_SIZE=32768` kB each, oldest evicted
  first; a manual `-packetsave` still takes precedence, and `-packetload` is now a replay viewer with
  a free camera and Tab cycling through the human players, with the console usable during playback
  and chat messages replayed deterministically. Multiplayer can no longer be won by proposing
  alliances, and the non-scripted victory checks that produced surprise wins in co-op are gone. The
  OpenGL renderer packs sprites with atlas UVs so edges no longer bleed. The configurable command
  character is gone (it was a workaround for broken keyboards). Lua: `StandIn` is now `Placeholder`,
  `AddShotToLevel` takes a parent, and things moved by script no longer interpolate across the jump.
  Two multiplayer maps get their creature attraction back. The `player->is_active` flag was
  refactored away with no gameplay change.
- **Upstream's script-command union fix, made right for a 64-bit build.** Their #5357 moves the
  x/y/icon of positioned messages to bytes 8–13 of the script value, which clears the location on
  their 32-bit Windows build (bytes 4–7) but lands exactly on it here, where `long` is 8 bytes and
  the location sits in bytes 8–15. This build takes their short slots and keeps the location where
  the fork already moved it (bytes 16–23), so both are clear. The fork's rule that only
  `DISPLAY_INFORMATION` carries an icon argument stays: upstream still reads the y coordinate of
  `DISPLAY_INFORMATION_WITH_POS` as an icon name. Proven with the positioned-message test map: the
  green information icon, not a random panel sprite.
- **Two config keys of ours moved again.** Upstream took ids 52 and 53 for the replay settings, on
  top of 51 and 52 last week; `TRASH_MAX_COUNT` and `TRASH_MAX_DAYS` now sit at 55 and 56.
  Existing config files need no edit. `COMMAND_CHAR` and `PACKETSAVE_MAX_SIZE` leave the shipped
  config; an old file still carrying them logs one "deprecated" line and carries on.
- **Verified** with the headless suites against stable 5656: the sync check (12/12), the common
  regression suite (same warning set as stable), the new-paths suite (all pass), the OpenGL smoke
  test, a twenty-second sweep of every campaign's levels (all alive), a replay autosaved on a menu
  quit, and the positioned-message proof map.

## 1.4.0.5732 — 2026-09-21 — alpha

- **Saved games from 1.4.0.5700 and from every stable build will not load in this build.** Upstream
  flattened the union inside `struct Room` (each room now carries its storage capacity, entrance
  links, per-model lair counts and hatchery turn side by side instead of sharing one slot), moved
  two roomspace fields out of `struct PlayerInfo`, and dropped an unused byte from `struct Game`.
  The saved blob is that struct verbatim, so its length changed (53362370 → 53383086 bytes) and the
  loader refuses older saves with the usual warning instead of guessing. Start new games; the saves
  are not touched.
- **Upstream sync: dkfans master through 7ca23d191 (26 commits, 16–21 September).** Multiplayer:
  players who drop are handled without desyncing the rest, replays and packet saves carry a format
  version and `PACKETSAVE_MAX_SIZE` caps the recording, the "out of sync" log spam is gone, dig
  tasks are checksummed, STUN keepalives and a rewritten hole punch keep connections alive, the
  roomspace flicker on Ctrl+wheel is fixed, and Lua state serialises correctly. Gameplay: things
  placed by script land at the right height, script-moved creatures start in a sane state, the sell
  button resets on map start, Lua `StartMoney` replaces gold instead of adding to it, empty tooltips
  are fixed and the Abyss slab has one, Lua can define room capacity functions. Renderer: OpenGL
  fixes part 5 with the text clipping and objective-length fixes, and semi-transparent panels fill
  their corners. The log now lists the audio devices. Creature target search was refactored for
  speed; its profiling hooks are compiled out here.
- **Kept our own defaults where upstream changed theirs.** Upstream's shipped config now defaults to
  the OpenGL renderer; this build keeps `RENDERER=SOFTWARE` until their fix series (part 5 of 8)
  ends — OpenGL remains one config line away, and the launcher now has a dropdown for it.
  `UNLOCK_CURSOR_WHEN_GAME_PAUSED` defaults to on, as upstream's does now.
- **The parchment map fade is a setting, off by default (`PARCHMENT_MAP_FADE`).** Upstream added the
  switch and removed the rule that skipped the fade above 320 pixels wide on the software renderer;
  that rule is kept here, so turning the fade on at desktop resolution under the software renderer
  still cuts instantly instead of running the slow per-pixel path. Two config keys of ours moved to
  make room for upstream's new ones; existing config files need no edit.
- **Launcher:** a **Renderer** dropdown (Software or OpenGL) and a **Fade when opening the map**
  checkbox on the Graphics tab, reading and writing the game config; a config without a renderer
  line shows Software, as the engine treats it. The launcher binary carries its version in an ELF
  section for crash reports and packaging.

## 1.4.0.5700 — 2026-09-20 — alpha

- **A defeated keeper's creatures no longer poison the room worker lists.** When a keeper's last
  heart dies the engine turns every creature it owned towards the nearest portal — without leaving
  the state it was in. A creature that was training (or researching, praying, imprisoned) kept its
  place in that room's worker list; once it was killed on the way out, or reached the portal, its
  slot was freed with the room still chaining through it. Every list walk from then on stopped at
  the dead node ("Jump to invalid creature"), the workers behind it were orphaned pointing at a
  room index that a bridge or workshop later reused, and the game spent the rest of the session
  logging creatures working in rooms of the wrong kind. Read out of a stable-5652 log: enemy heart
  destroyed at turn 315366, the first dead node found in that keeper's training room 1600 turns
  later, then 500 warnings about trainees and crafters "in" bridges. Reproduced with a scripted
  level (a creature training when its keeper falls, then killed on the way out) and fixed at the
  source: the redirect now runs the abandoned state's cleanup first, as the over-population path
  already did. Upstream has the same gap.
- **Two safety nets behind it.** A creature deleted while still in a worker list is unlinked at
  deletion and the log names the state that let it through; and a room being deleted severs every
  control still naming it, chain or no chain, so a reused room index can never inherit workers.
  The 5652 list guards remain the last line. Neither fires on a healthy game.
- **A creature preparing to cast a ranged buff (Ranged Heal and kin) is cleaned up as what it was
  doing.** The preparation state parks the real state aside and restores it afterwards, but state
  cleanup only looked at the preparation itself — a worker killed or interrupted mid-cast stayed
  chained into its room. Same class of leak, different door; none of the stock creatures use these
  spells, custom campaigns do.
- **Room rebuilds no longer push workers into the wrong room kind.** Selling a slab out of a room
  re-homes its workers into whatever room is under their feet; a trainee standing on the bridge
  next door was listed as that bridge's worker, still training, until the job handler noticed and
  reset it. Only a room of the same kind is accepted now. This was the source of the "Room BRIDGE
  index 39 is not valid ROOM_ROLE_TRAIN_EXP" storm (339 lines in one session).
- **Imps no longer drop into state 0 after resting.** An imp that walked to safety to recover, then
  found itself healed, was restored to "no state" and reset the next turn ("illegal state[1], S=0"
  in the log). It picks a proper start state instead.

## 1.4.0.5698 — 2026-09-20 — alpha

- **Positioned script messages no longer wear random icons.** `DISPLAY_INFORMATION_WITH_POS(text, x, y)`
  fed its y coordinate to the custom-icon parser — an upstream slip in the custom-icon feature, and
  `get_icon_id` on a number is just `atoi` — so a message placed at y=154 showed the alarm-trap
  lightning bolt, at y=112 the Hold Audience gate, at y=55 the guard post: the wrong picture, drawn
  at its own size, spilling out of the message column and blinking while unread. Found in a tutorial
  map; every map that places information messages was affected. The icon is read only for the form
  that takes one. Saved games are unaffected.

## 1.4.0.5696 — 2026-09-16 — alpha

**Upstream sync — 33 commits.** The headline is upstream's new **OpenGL renderer**: world, UI,
sprites, shadows and lens effects drawn by the GPU, behind the same renderer seam the software
path uses. Upstream only wires it into their Windows build; this release compiles it into the
Linux binary as an **opt-in** — set `RENDERER=OPENGL` in `keeperfx.cfg` to try it, `SOFTWARE`
stays the default. Also from upstream: multiplayer hole-punching now learns the peer's real port
and the lobby waits five seconds instead of two; the startup-sync render crash is fixed; creatures
no longer walk onto abyss or lava (three fixes deep); a creature teleporting to a battle no longer
lands in the map corner; the battle list no longer shows the same fight twice and cycles with fewer
than four battles; lights no longer flicker at map edges; the parchment map keeps its audio and no
longer interpolates a jump; more Lua fields; and the per-user game state moved out of the
per-player record — taken as upstream shipped them.

- **The fork's own GPU present layer is retired.** Since June the game's 8-bit frame was uploaded
  to the GPU and palette-mapped in a shader. Upstream's renderer has since grown a command stream,
  a render thread and its own display surface, so that thin adapter could only have survived as a
  copy of the software renderer with a different final blit. The software renderer now presents
  through SDL exactly as upstream's does — and measured on the same level, main-thread CPU went
  **down** (74% → 54%), so nothing was lost. With it go two lines of the earlier changelogs: the
  GPU palette re-upload guard and the resolution-blackout fix, both of which only existed for that
  layer. `libepoxy` is no longer a build or runtime dependency (source builds and the Arch package).
- **Kept: the parchment map cache.** Upstream now presents the map background through the renderer,
  which rescales it on every frame — the exact cost this fork removed. The cache is re-applied
  around upstream's call and used whenever the software renderer is active.
- **Kept: click-away text entry, the window icon, mouse focus.** Upstream added its own click-away
  (which commits the field and needed a per-button opt-out for save names); this fork's version,
  which leaves the field without committing, stays. Upstream's Linux window icon is compiled in by a
  CMake step this build does not have, so the icon is loaded from `fxdata/` at upstream's new hook
  instead. Cursor-in-window still uses mouse focus (the global cursor query returns 0,0 on Wayland).
- **Movies in any codec still play** — a non-Smacker source is converted to a 256-colour frame and
  presented through the renderer; the truecolor path went with the GPU layer.

**Also since 5652, on this fork:**

- **Launcher settings.** The **Rotate around cursor** control (the engine has had the setting since
  July); zoom-to-cursor writes the values the engine actually reads (`ON`/`OFF`/`WHEEL`); every
  boolean game setting is parsed case-insensitively; moving a slider enables Save.

**Saved games from build 5652 and earlier cannot be loaded by this release.** Upstream moved the
per-user state (cursor, possession, chosen room and spell, battle selection) out of the per-player
record and into its own table inside the saved game state. See the warning below, or use the
game's own "keep the previous engine" feature to finish anything in progress first.

## 1.4.0.5652 — 2026-09-07 — stable

**Replaces 1.4.0.5425 as the stable release.** Everything the alpha line accumulated between
5459 and 5649 (below), promoted after a full pass: unit tests under the sanitizers, a sanitizer
run of every campaign's first level, the save/load/delete, multiplayer, level-coverage,
resolution-switch and launcher-update suites against the 5425 engine as baseline, and the
AppImage started on Ubuntu 24.04, Debian 13, Fedora 41 and Arch. In short:

- **SDL3.** The engine moved off SDL2 in 5459; window, input and audio go through SDL3 now,
  with the fork's cursor, sub-pixel mouse motion and fullscreen-transition fixes on top.
- **Saved games survive an engine update.** A save the running build cannot read is refused
  with a message instead of ending the game; the engine that wrote it is kept alongside
  (`keeperfx-tux-previous` on Arch, the launcher's own stash elsewhere); deleting a save moves
  it to the trash; the save list packs itself; and the release build refuses to ship if the
  game-state layout changed without saying so.
- **Stability.** The creature-list corruption behind the level-transition abort is fixed at the
  root, four memory faults found by the sanitizers are fixed, and changing resolution no longer
  blacks out the game.
- **Rendering.** The fork's GPU present path sits behind upstream's renderer seam, honours the
  vsync setting, and takes screenshots.
- **Launcher and packaging.** Workshop and Install… take `.rar` add-ons; the Arch package ships
  the archive library the launcher needs; the AppImage bundles the three libraries Fedora and
  Debian do not carry; the alpha and stable channels are kept apart by the updater.
- **Upstream.** Three syncs (5482, 5596, 5649) — custom icons for script messages, the Abyss
  terrain, custom ensigns, pathfinding and multiplayer input-lag work, and much else, taken as
  upstream shipped them with this fork's fixes kept underneath.

**Saved games from 1.4.0.5425 cannot be loaded by this release.** The game-state layout changed
three times along the alpha line. On Arch the 5425 engine is kept by the upgrade hook and runs
with `keeperfx-tux-previous 1.4.0.5425`; the launcher keeps the replaced engine the same way.
The release notes carry the warning automatically.

Known: on Fedora 41 the AppImage's engine crashes instead of showing its dialog if startup has
already failed fatally (a fault inside the bundled SDL3's message box); normal play is
unaffected.

## 1.4.0.5649 — 2026-09-07 — alpha

**Upstream sync — 46 commits.** Dungeons can now reach into the Abyss (a new terrain type;
creatures and things that fall in are lost, with a Lua hook to rescue them); pathfinding no longer
stalls on unreachable jobs or on paths behind lava; the multiplayer input-lag controller now
samples by time rather than by turn, and packet-history repair is spread across peers; custom
ensigns for campaign and multiplayer maps; transformed creatures keep their lair and health;
computer players sacrifice imps according to `keepcompp.cfg`; the `TRIGGER_ACTION_POINT` and
`DISPLAY_VARIABLE_WITH_LABEL` script commands; level-load timing in the log; and a run of smaller
combat, effect, wibble and Lua fixes — taken as upstream shipped them.

- **New settings, same names on Linux.** `CAPTURE_CURSOR=OFF` in `keeperfx.cfg` is now the config
  equivalent of `-altinput`; `MATCHMAKING_SERVER` and `MULTIPLAYER_PORT` are configurable; `-nick`
  sets the multiplayer name and `-waitusers` the lobby size from the command line.
- **`SKIP_HEART_ZOOM` left the config file.** Upstream moved it to a `-skipheartzoom` command-line
  flag; a value still in `keeperfx.cfg` is ignored (with a log line), not an error. `-level` now
  skips the splash screens as well.
- **Kept: the sprite-decoder bounds fix.** Upstream's new shared PNG decoder — which now also feeds
  the ensign loader for map zips — sized its buffer with the pre-hardening formula this fork had
  already replaced. The worst-case size is re-applied inside it, so a crafted archive cannot
  overflow the sprite buffer.
- **Renderer sources moved.** Upstream relocated the software raster under
  `src/kfx/renderer/software/` behind a routing layer; this fork's GPU present path bypasses the
  layer and its cost was measured at under 0.05% of samples either way.

**Also since 5596, on this fork:**

- **RAR add-ons install.** The launcher's Install… and the Workshop browser now handle `.rar`
  archives (workshop items such as *Infernal Rift* ship that way), detecting the format from the
  file rather than its name. The Arch package now ships the archive library the launcher needs;
  fresh `yay -S keeperfx-tux` installs previously had no archive support at all — no workshop
  installs and no self-update — unless a leftover from the AppImage era happened to be present.
- **AppImage starts on Fedora and Debian.** Three libraries that bundled dependencies need
  (`libgpg-error`, `libcom_err`, `libgmp`) were left to the host and are absent on Fedora 41 and
  Debian 13. They are bundled now, and the build warns about any future unresolved dependency.
- **Unit-test link restored** after upstream's platform-seam move had silently taken the weekly
  sanitizer job down since 2026-08-24.

**Saved games from build 5596 and earlier cannot be loaded by this release.** Upstream moved
local-only view state out of the per-player record and widened the campaign's next-level field,
both inside the saved game state. See the warning below, or use the game's own "keep the
previous engine" feature to finish anything in progress first.

## 1.4.0.5596 — 2026-08-25 — alpha

**Upstream sync — 23 commits.** Custom-icon support for message and objective/information
script commands, unlimited possible lair enemies, a 17% engine performance improvement,
several multiplayer fixes (chat, lobby permanence, input lag, getting stuck in parchment
view), Spanish translation for The Good Campaign, and a handful of smaller combat/UI/creature
fixes — taken as upstream shipped them.

- **Rotate-around-mouse simplified.** Upstream collapsed the option down to ON/OFF plus
  ROTATION_KEYS/MOVEMENT_KEYS (dropping the separate FOLLOW/NO_FOLLOW flag) and reset the
  shipped default to OFF. We'd previously defaulted this fork to on; upstream's own commit
  message flagged the reset as a deliberate correction, so we followed it rather than keep
  our prior default.
- **Kept: the message/objective location fix from build 937eeb15.** Upstream's new
  custom-icon code reads a script command's saved location from a union offset that overlaps
  adjacent fields on this fork's 64-bit build — the same class of bug already fixed once for
  `QUICK_MESSAGE`/`DISPLAY_MESSAGE`. Took upstream's new icon-aware functions, kept this
  fork's safe offsets underneath them.

**Saved games from build 5554 and earlier cannot be loaded by this release.** Upstream's
custom-icon feature added a field to every event/objective marker, which is part of the save.
See the warning below, or use the game's own "keep the previous engine" feature to finish
anything in progress first.

## 1.4.0.5554 — 2026-08-21 — alpha

**Updating the game no longer costs you your saved games.** A save is a raw copy of the
engine's internal state, so one field added anywhere inside it makes every existing save
unreadable — the bytes are intact, they simply mean something different to the newer
engine. That happened between 1.4.0.5488 and this line: a single 4-byte field added
upstream, multiplied by the 12,288 objects the engine tracks, moved the save format by 49KB
and campaigns stopped loading with no warning. Three changes make that the last time.

- **The game tells you, instead of vanishing.** Loading an incompatible save used to quit
  the game outright — the window disappeared, with the reason only in the log. It now says
  *"Saved by another game version — play it with that version"* and returns you to the
  menu. The check runs before anything is loaded, so a refusal no longer leaves the session
  half-switched to another campaign.
- **The engine that wrote your save is kept.** Upgrading preserves the outgoing engine,
  three releases deep. `keeperfx-tux-previous` lists what is kept and prepares an isolated
  copy of the game to play one — its own save directory, holding only the saves that
  version can read. It cannot write to your current game. The launcher does the same for
  installs it updates itself.
- **A release that breaks saves cannot ship unnoticed.** The build measures the save format
  and compares it against a recorded baseline. If it moves, the release fails and has to be
  accepted deliberately, and the notes say plainly that older saves will not load. The same
  check runs on incoming upstream merges, which is where this one arrived from.

**Deleting a saved game is no longer permanent.** Delete moves it to your desktop Trash,
where *Restore* puts it back — the freedesktop standard, so your own file manager handles
it. Where a real Trash is unreachable, such as a sandboxed Flatpak, the game keeps its own:
the 10 most recent deletions for 30 days, whichever limit is reached first, both
configurable with `TRASH_MAX_COUNT` and `TRASH_MAX_DAYS`.

- The save list closes up when you delete, instead of leaving an `UNUSED` hole in the
  middle of it.
- Panel and button artwork missing from a loaded sheet is skipped rather than drawn as a
  magenta checkerboard — which is what upstream's new possession-panel arrow looked like.
- A malformed line in `keeperfx.cfg` is now reported and ignored, instead of silently
  adopting an unrelated value from the line before it.

**The fork's GPU present path now sits behind upstream's renderer seam.** Upstream
introduced an `IRenderer` abstraction; this fork's OpenGL path — which is roughly ten times
faster at getting a frame to the screen than the software blit — now lives behind it rather
than beside it. That keeps the two in step instead of diverging, and leaves the door open
for a macOS build: the GL context is created forward-compatible, which macOS requires.
Screenshots work on the GL backend, and the VSync setting is honoured rather than ignored.

Merged from upstream KeeperFX: a scrollbar and unlimited slots in the save/load menus,
minimap zoom keys, a togglable relative mouse mode, more reliable multiplayer server
messaging and lobby status, configurable boulder-destroys-room, allies kept safe from
disease, and fixes for rectangular-map tagging, damage flashing and distorted panel icons.

## 1.4.0.5488 — 2026-08-10 — alpha

**The creature-list corruption is fixed at the root.** The crash this fork guarded against
in 1.4.0.5322 — the game aborting between campaign levels after "invalid creature"
warnings — is now understood end to end and fixed at its cause, not its symptom.

- **What it was:** deleting a room never unlinked the creatures still working in it, and a
  creature leaving its room trusted "no previous neighbour" to mean it led the room's list.
  Defeat an enemy keeper, claim their territory, and a dead room's worker chain could be
  spliced wholesale into one of your healthy rooms — corruption planted silently, detected
  thousands of turns later, fatal only at a level transition after victory. It took one
  recorded crash, its full log, and five days to reconstruct.
- **What changed:** rooms now sever every worker when they are deleted; the list head is
  only rewritten when the room actually agrees the creature leads it, and a refusal is
  logged loudly rather than obeyed; a creature whose room is already gone detaches itself
  instead of warning forever. The eighteen guarded list walks from 1.4.0.5322 stay, as the
  last line of defence rather than the only one.
- The fault dates from 2009 and is present in upstream KeeperFX today.

## 1.4.0.5485 — 2026-08-10 — alpha

**Four memory faults fixed, found by turning on the compiler's own alarms.** The engine
now has a memory-checking build (AddressSanitizer) and a script that runs every installed
campaign under it; its first afternoon caught four faults that months of play never
showed, all inherited from the original project and all present since well before this
fork's first release.

- **The colour table wrote past its own end on every game start.** One array held ten
  entries while the code filling it wrote eleven — it has been like that since the
  purple, orange and black player colours were added. The stray write landed on the
  dungeon heart's beat-animation state, so the practical symptom was at most a heartbeat
  wobble; the fix sizes the array from the same constant the filler uses, so the two can
  never drift apart again.
- **The level-script reader looked one letter past the end of every command name** shorter
  than the word it was checking, on nearly every campaign. Read-only and harmless in
  practice, now simply correct.
- **Tiny map files were read past their end on every level load.** Some map files are
  legitimately a single byte; the decompressor read an eighteen-byte header out of them
  regardless. Files too small to be compressed are now treated as what they are.
- **Finished sound messages were destroyed as the wrong type** — a one-line C++ mistake
  (a missing virtual destructor) that made every retired message undefined behaviour.

None of these was dangerous to your saves, but all four were real, and the tooling that
found them is now part of the project: a build mode any developer can run, a weekly
check that keeps it working, and a compiler setting that turns an entire class of bug —
a failure result silently ignored — into a build error from now on.

## 1.4.0.5482 — 2026-08-10 — alpha

**Upstream sync.** Three changes from the KeeperFX team, one of which needed care.

- **Creatures that were asleep on payday now get the gold you handed them.** An advance
  taken while a creature was in a non-salary state — sleeping, most often — did not take
  effect until later. Upstream's fix, taken as-is.
- **File enumeration and OS-version queries moved into the platform layer**, continuing the
  rework this fork already adopted. Both are internal tidying you will not see.
- **Kept: hidden files stay invisible to the game.** Upstream's new enumeration skips only
  `.` and `..`, where this fork skips every hidden entry. That matters on Linux: campaigns,
  mods and workshop content usually arrive as archives built on macOS or Windows, which
  carry `._name.cfg` sidecar files beside the real ones. Without the filter the game reads
  those as real campaigns and mods — and because `.` sorts first, the sidecar is picked
  ahead of the file it shadows. The filter was carried into the new code; verified by
  planting sidecars and confirming the engine ignores them.

## 1.4.0.5475 — 2026-08-10 — alpha

**Changing resolution no longer blacks out the game.** This one also affects the current
stable release, 1.4.0.5423.

- **Fixed: the screen went black, permanently, after changing resolution in the in-game
  Graphics menu.** The game kept running — it was still drawing and presenting frames at
  full speed, they were simply black — and nothing brought it back: no further resolution
  change, no key, no menu. The 8-bit picture is turned into colour on the GPU using a
  256-colour lookup table. Uploading that table every frame was wasted work, since it only
  changes on fades, flashes and movies, so it is uploaded only when it differs from the last
  one sent. Changing resolution rebuilds the graphics backend, which creates a fresh, empty
  lookup table — but the game's own palette either side of that rebuild is identical, so the
  check concluded there was nothing to send. Every pixel then looked itself up in a table
  full of zeroes, which is black, and it stayed black because the palette never genuinely
  changed again. The table is now refilled whenever the backend is rebuilt. Switching
  between borderless fullscreen, exclusive fullscreen and your own windowed resolutions all
  work again.

## 1.4.0.5473 — 2026-08-09 — alpha

**Follow-up to the save delete button.** Engine-side; the launcher is unchanged.

- **The delete button explains itself.** Hovering the skull now reads *Delete this saved
  game*, in all sixteen languages the game ships. The wording follows the engine's own
  tooltip convention of a short heading before a colon — that is not cosmetic: the tooltip
  box is sized to whatever comes before the colon, and without one the box sizes itself to
  the whole sentence and runs off the right edge of the screen, since these buttons sit in
  the menu's right margin.
- **Clicking away from a name you are typing no longer freezes the menu.** Clicking a save
  row starts editing its name, and while any text field is being edited the game gates out
  every button on screen. So clicking a row and then changing your mind left the menu
  unresponsive with nothing to explain why, and clicking again did not help — only Enter or
  Escape did. This has always been the case; the delete buttons simply made it easy to run
  into, because there was previously nothing else on that menu to click. A click outside the
  field now ends the edit and still reaches the button you aimed at, in one click. What you
  had typed is kept but not saved, so nothing is committed and nothing is thrown away. This
  applies to every text field in the game, not only the save menu.

## 1.4.0.5470 — 2026-08-09 — alpha

**You can delete a saved game.** Engine-side; the launcher is unchanged from 1.4.0.5467.

- **A delete button on every save slot.** There are eight save slots, and until now the only
  way to reclaim one was to overwrite it. That is fine until you are mid-campaign and the
  names of the games you saved weeks ago no longer mean anything — you have to overwrite
  something without knowing what you are destroying. Nothing in the engine deleted a saved
  game at all. Each row on the in-game Save and Load menus now carries a skull button in
  the right margin, using Bullfrog's own "dead keeper" symbol. It appears only on slots that
  hold a game, and it asks before doing anything. The confirmation **names the save it is
  about to delete**: the dialog covers the list, and telling apart saves you can no longer
  identify is the whole point. Confirm and the slot frees, the row reverts to `UNUSED` and
  the menu stays open, so you can clear a couple of slots and save without leaving for the
  dungeon and coming back.

## 1.4.0.5467 — 2026-08-08 — alpha

**A launcher fix for the map editor.** The engine is unchanged from 1.4.0.5465.

- **The map editor is kept up to date.** The launcher used to install Unearth once and
  then only ever check that it was still there, so whichever version you first installed
  is the one you kept — indefinitely. That mattered more than it sounds: an install from
  before the start of July predates the release where Unearth learned to find a native
  Linux KeeperFX. Until then its file picker looked only for `keeperfx.exe`, so on Linux
  it could not see the game at all, and nothing would ever have offered you the version
  that fixes it. The launcher now remembers which release it installed and offers the
  newer one when you open the editor.

## 1.4.0.5465 — 2026-08-08 — alpha

**Mostly the launcher this time.** The engine gains upstream's windowing rework and
little else; everything you are likely to notice is in the launcher.

- **Your Tux Edition releases now appear in the launcher.** The "Latest News" panel reads
  keeperfx.net, which reports the Windows project's releases — so this launcher was
  updating you to builds it never mentioned. The newest Tux Edition releases now sit at the
  top of that panel, above the KeeperFX team's news, which is still there: their engine
  changes reach you too.
- **You can read the logs without leaving the launcher.** Play ▾ → **View logs** opens the
  launcher log and the game log side by side, with a Copy button — the usual next step when
  reporting a problem — and it keeps following the file while it is open, so you can
  reproduce a fault and watch it happen. When something fails, the error itself now offers
  a **Show log** button.
- **Workshop items that the site publishes but its API does not are installable again.**
  Some items — those needing an unreleased game version — are served perfectly well by
  keeperfx.net while its API reports no file for them at all. The launcher used to say
  "Could not get a download link", which reads like a fault at our end. It now falls back
  to the item's own page. Where genuinely nothing is published, it says so, and says where
  the gap is.
- **The window resizes properly.** It was already draggable on Wayland — the code that
  tried to prevent it has no effect there — but nothing moved when you did, so it only
  looked broken. Now the workshop panel lays its cards out across as many columns as fit,
  and the window opens wide enough to show four of them in a row.

And in the engine:

- **Upstream's windowing and platform rework**, taken so this fork does not drift from a
  file both projects edit heavily. It cost us nothing you can see: the OpenGL support their
  version had no way to express is carried on top of it, and a fault it reintroduced — the
  DESKTOP_FULL video mode quietly losing its real mode switch — is fixed again.
- **Multiplayer and screenshots on SDL3**, from the KeeperFX team.

## 1.4.0.5459 — 2026-08-08 — alpha

**The engine now runs on SDL3.** The KeeperFX team moved off SDL2, and this brings that
across. SDL is the layer between the game and your desktop — window, input, sound — so
this touches everything, which is why it lands on the alpha channel first and not on
stable. Nothing about it should be visible to you: the same resolutions, the same
controls, the same audio. If anything behaves differently from 1.4.0.5423, that is worth
reporting, because it is not intended.

Five faults were found reviewing the port before it shipped. Two of them fail silently,
which is the kind that otherwise reaches you and gets blamed on something else:

- **Winning a level could crash the game.** The cutscene where the Lord is dragged to the
  torture chamber was missing from the frontend's state handling, so completing a level
  that unlocks it could take the game down instead. That fault has been there since the
  sequence was added in 2019.
- **Slow mouse movement was being thrown away.** SDL3 reports pointer motion in
  fractions of a pixel where SDL2 used whole numbers, and the fractions were being
  discarded rather than accumulated. On Wayland, on a HiDPI screen, or with pointer
  acceleration, a slow drag could register as no movement at all.
- **"DESKTOP_FULL" had quietly stopped being a real fullscreen mode.** It had become
  identical to "DESKTOP" — a borderless window — so choosing it no longer changed the
  display mode as it says it does.
- **Leaving fullscreen could strand the window at the wrong size,** because SDL3 applies
  that change in the background where SDL2 applied it immediately, and the game resized
  the window before the change had happened.
- **Music could go silent for the rest of the session.** If a track failed to start, the
  previously playing track had already been discarded, and every later request to play it
  again was treated as already-playing and did nothing.

Also in, from the KeeperFX team:

- **Hand rules now apply to prisoners,** not only to your own creatures.
- **The dig-tagging sound always plays in multiplayer,** rather than being swallowed when
  the game was predicting your dig locally.
- **A possible crash in the drag animation** on the frontend.
- **Creature names are now written as comments in the configuration files,** which makes
  them far easier to edit by hand.

Behind the scenes, the release pipeline builds SDL3 from source rather than moving to a
newer Ubuntu, so the AppImage keeps running on every distribution it ran on before.

## 1.4.0.5423 — 2026-08-05 — stable

**Replaces 1.4.0.5411 as the stable release.** The engine is unchanged; all of this is
the launcher. A review of it after 5411 went out found thirteen faults, and the ones that
mattered were in the two things people do most: updating, and installing add-ons.

- **Add-ons from the Workshop install again.** Every file whose name contains a space —
  a large share of the catalogue — failed to download, because the site hands out a link
  its own server rejects. And a campaign, which ships as a bare `name.cfg` beside a
  `name/` folder, was not recognised at all: it was mistaken for a loose map pack, so its
  levels were scattered into Personal Levels, its campaign file and creature, land and
  media folders were dropped, and it reported success. Campaigns, map packs, single maps,
  creature mods and graphics packs were each tested against a real download from the
  Workshop, and each now lands where it belongs.
- **Where an add-on goes is read from the add-on itself.** Its configuration says whether
  it is a campaign or a map pack, which the folder layout cannot tell apart — and which
  the Workshop's own category sometimes gets wrong.
- **A failed install says so.** Copying into a folder it could not write to reported a
  successful install and wrote nothing. On an Arch install the game's data folders belong
  to the package manager, so that was every add-on install. Those folders now allow your
  own content alongside the packaged content, and a genuine failure explains itself.
- **Uninstall and Restore no longer fail silently — and Restore cannot lose your add-on.**
  It moved files back, ignored whether the move worked, then deleted the backup and its
  record regardless. If it could not write, the add-on was gone from the game and gone
  from the backup. The backup is now kept whenever anything fails.
- **A damaged download is noticed.** The check for a corrupt archive could never fire, so
  a truncated file went straight to being unpacked.
- **Opening Settings with four or more resolutions configured is no longer undefined.**
- **When something fails, it tells you why.** "Could not download" now includes the reason.

## 1.4.0.5411 — 2026-08-05 — first stable

**The first stable release of the Tux Edition.** Everything here has already shipped
through the alpha channel; what is new is the promise. The alpha channel keeps moving —
the SDL3 migration is being prepared there now — while stable stays put until a release
has been played and left alone for a while.

- **Same engine as 1.4.0.5397**, which had been soaked in ordinary play with nothing to
  report, and which starts and runs every one of the 13 installed campaigns — checked one
  at a time, each on its own first level.
- **Two update channels, kept apart.** The launcher now asks for the newest release *of
  its own channel* rather than the newest release overall. Choose stable and alphas will
  not be offered to you; choose alpha and you keep getting them. Previously there was one
  list and whichever release was newest won, which would have made a stable channel
  meaningless the moment the next alpha appeared.
- **Stable releases keep their build number.** A stable was being read as `1.4.0`, dropping
  the part that actually identifies the build, so an install compared older than the very
  release it was running and was offered an endless update to itself. Version ordering was
  also wrong in a way that could have offered a downgrade: a large build number on an older
  base counted as newer.
- **The launcher no longer offers updates it cannot install.** On a package-managed install
  — the Arch package, where the engine and data are root-owned system files — it now says so
  and points you at your package manager, instead of downloading 400 MB and failing with a
  permission error that explained nothing.

## 1.4.0.5397 — 2026-08-05
- **The game now paces itself to your monitor instead of running flat out.** A new install drew frames as
  fast as the processor allowed — frames the display never showed, with one core pinned at 100%. Worse, it
  left nothing in reserve: the game runs on a single thread, so a frame that needed more work than usual
  overran and was dropped. That is the intermittent stutter with no obvious cause. Drawing is now capped at
  the refresh rate your display reports, and when the machine cannot hold that rate the cap steps down to a
  whole fraction of it (144 → 72 → 48), which turns a dropped frame into one that still arrives on time. It
  settles rather than hunting, and steps back up once the headroom returns. A rate you set yourself in the
  configuration is an instruction and is never overridden. Multiplayer and replays are unaffected — the game
  advances at its own pace regardless of how often the screen is drawn.
- **The map view no longer costs a whole processor core.** With the map open, 46% of all CPU time went on
  stretching the parchment background to fill the screen — five million pixels at 3440×1440, every frame, on
  the one thread the game runs on — only to discard the result and do it again. That image is static; only
  what is drawn on top of it moves. It is now scaled once and kept, and rebuilt only when something it
  depends on genuinely changes. The wider your display, the more you get back.
- **Files that came along for the ride no longer appear as campaigns, mods and levels.** Archives built on
  macOS or Windows carry a small hidden companion file beside every real one, and campaigns and mods reach a
  Linux install as archives. The game was treating those companions as content: reading them as campaigns,
  handing them to the graphics loader, counting them as levels, and telling you on-screen to install one as
  a mod. They also sort ahead of the file they shadow, so anything choosing by position chose the wrong one.
  Hidden files are now ignored everywhere the game looks for content.

## 1.4.0.5384 — 2026-08-03
- **Multiplayer map packs work.** Classic, Modern and Original now appear in the multiplayer menu with
  their maps, in every install method. They are declared by text files this repository tracks, but the
  maps themselves live in the game data — and the update archive is assembled from the *previous*
  release's archive, so a directory that did not exist when that chain began could never join it. The
  mappacks arrived upstream a month after the 1.3.2 data this fork first packaged, which is exactly the
  gap they fell into. The archive is now layered over the KeeperFX team's current data package on every
  build, so their new content arrives on its own instead of waiting to be noticed.
- **The multiplayer service list reads correctly again.** It offered "Serial", "Modem" and "IPX" — the
  1997 game's options — where the engine actually builds "Online", "LAN" and "Skirmish". The names come
  from a string table that had drifted out of step with the engine reading it, for the same reason as
  above. Same fix, and it keeps the rest of the interface's text current too.
- **Campaigns and levels are current again.** Their configuration had been frozen at whatever the archive
  first shipped: 14 of 60 campaign files and 22 of 60 level files were behind the engine, including
  changes the team made in July. Nothing you need to do — an update simply brings them in step.
- **Install from the AUR on Arch.** `yay -S keeperfx-tux` builds the engine against your own system's
  libraries and pulls the game data in with it, so one command is the whole install and `yay -Syu` keeps
  it current. The AppImage stays for everything else, and the Flatpak still updates monthly.
- **The game window has an icon and a name.** It showed a generic placeholder in taskbars and docks under
  both X11 and Wayland; the launcher window did too.

## 1.4.0.5366 — 2026-08-03
- **`KFX_NONINTERACTIVE=1` for scripts and CI.** A startup problem — an unrecognised command-line option,
  say — raised a message box and waited for a click. Fine for a person, fatal for anything automated: CI,
  scripts, and the launcher's *extra launch options* field all hung until something timed out, with the
  reason visible only in the log. Set this variable and the engine logs the message and carries on. Unset,
  nothing changes, so ordinary players still get their dialogs.
- **Our own `music/MusicReadme.txt` now actually reaches you.** Shipping it in 5363 wasn't enough: the
  update archive is assembled from the *previous* release's payload, and its config overlay copied
  `fxdata`, `creatrs` and `mods` but not `music` — so a file only the engine repo knew about would have
  stayed frozen indefinitely. Caught by checking the readme's size inside the built archive before
  publishing. The same freezing bit this fork once before, which is why that overlay exists at all.

## 1.4.0.5363 — 2026-08-03
- **Your own music, any names, any format.** The game used to demand exactly `keeper02.ogg` …
  `keeper07.ogg` and play nothing otherwise. Now it uses whatever audio is in `music/`: name the files
  what you like (`Track 02.flac`, `02.wav`, or the original names), in **OGG, FLAC, WAV or MP3**. Files
  carrying a track number are placed by that number; files without one are used in alphabetical order.
  Nothing about an existing install changes — `keeperNN` names are still looked up first and still win,
  so the same files keep playing.
- **Silent music is no longer a mystery.** `keeperfx.log` now says how many tracks were found and names
  every file it did **not** use, with the reason — no track number, a better-quality copy of the same
  track winning, or more files present than there are tracks. Previously this was logged at a debug level
  no normal build emits, which is how a music folder holding nothing but its readme went unnoticed for
  weeks.
- **The launcher's "music missing" prompt actually reaches you.** It was only ever offered while copying
  the original DK files, so an otherwise-complete install could never be told its music was missing.
  It now checks at startup (with a "Don't show this again" box), and there's a **Download music** entry
  in the menu beside the Play button. The check also understands FLAC/WAV/MP3 soundtracks and symlinked
  music libraries, so a curated folder is never wrongly called empty — and a partly-failed copy now
  offers the download instead of quietly leaving you short a track.
- **Fixed: an OpenAL error at every startup.** `SetSoundMasterVolume: OpenAL error … Invalid operation`
  appeared in every log. The game window exists before the audio system starts, so the focus event fired
  on it reached the mixer too early. Cosmetic — audio was never affected — but the log is clean now.
- **`make` builds Linux.** A bare `make` ran the upstream Windows/mingw target, which also overwrote a
  shared dependency and left the next Linux build failing with a bare `cannot find -ljson`. It now builds
  Linux, and repairs that dependency automatically if a Windows build clobbered it.
- **A `music/` readme that is actually true.** The one shipped upstream still said "OGG format" only.
  Ours explains where to get the music, that filenames are free-form, which format to pick and why
  (OGG by default; FLAC only if you rip the CD yourself; MP3 avoided because its encoder padding makes an
  audible gap every time a track loops), and how to read the log when something isn't playing.
- **Upstream sync — 6 patches** (#5064–#5082): default campaign loaded when none is active (#5082), three
  new classic multiplayer maps (#5081), frameskip doubling (#5076), multiplayer lobby chat fixed (#5079),
  ctrl drag-click camera movement fixed (#5078), and a large `main.cpp` cleanup (#5064). All clean-merged.

## 1.4.0.5335 — 2026-08-01
- **Named: "KeeperFX — Tux Edition."** The native Linux build now has an identity — the game's main-menu
  stamp, the launcher window title, and the GNOME menu entry all read **KeeperFX Tux Edition**. (The repo
  slug, asset filenames and version string are intentionally unchanged for update-pipeline stability.)
- **Cleaner in-game menu.** The menu version stamp drops the volatile build number (it climbed every build);
  the exact version still lives in the launcher, `keeperfx.log`, and crash reports.
- **Cleaner README.** The three heaviest sections (full feature breakdown, upstream-sync explainer, DK1
  install guide) are now collapsible so newcomers aren't overwhelmed.

## 1.4.0.5332 — 2026-08-01
- **Weekly upstream sync (run early, by hand).** Pulled the KeeperFX team's latest **7 patches**
  (#5066–#5075) — a rendering-heavy batch that clean-merged with our fork:
  - **High-res / "straight view" rendering fixes** — a great batch for our ultrawide + 4K Linux setups:
    sprites no longer vanish at max zoom in 4K (#5066), units no longer show through walls (#5068), and
    door keys are no longer invisible (#5073), all in straight view; plus a rewrite of the polygon
    rasteriser `bflib_render_gpoly.c` (#5071).
  - **Smoother camera** — local camera controls are processed before the logic update (#5067) and
    catch-up packets handled after it (#5075).
  - **GUI** — message-button drop-animation fix (#5069).
- Also carries the launcher **Workshop instant-load caching** (thumbnails + catalogue cached to disk,
  refreshed in the background) that had been staged since 5322.

## 1.4.0.5322 — 2026-07-29
- **Crash hardening — creature-list corruption is now survivable.** A corrupted creature-list link (a
  "next" pointer aimed at a slot that isn't a creature) could crash the whole game with an abort — e.g.
  when advancing between campaign levels after the creature list got tangled mid-level (visible symptom:
  a creature resting away from its lair; log shows *"Jump to invalid creature N detected"*). The engine
  now validates each creature in a list before using it, in the ~18 walks that previously lacked that
  check; on a bad link it logs rich diagnostics (turn, list, slot, thing type) and safely stops that walk
  instead of aborting — the game keeps playing (at worst one glitched creature is skipped that frame).
  Purely additive guards, mirroring the ~50 the engine already had. This is a genuine engine bug present
  upstream too; the added logging is the path to a permanent root-cause fix if it recurs.

## 1.4.0.5321 — 2026-07-29
- **In-launcher Workshop browser (new).** A **Browse Workshop** button opens the full
  [keeperfx.net workshop](https://keeperfx.net/workshop) catalogue *inside* the launcher — search, filter by
  category, sort by rating / downloads / newest, thumbnails, and a **Details** link to each item's page — with
  **one-click install** for maps, campaigns, map packs and mods. A new **Installed** tab scans your add-ons,
  labels each as 🏰 KeeperFX-stock or 🌐 Workshop/user (filterable by category + name), and offers a
  **reversible Uninstall → Restore**: removed items move to a local backup you can Restore in one click or purge.
- **Standalone maps + cleaner installs.** Single workshop maps now install correctly (into `levels/personal`),
  and version-control / OS junk (`.git`, `__MACOSX`, `.DS_Store`) is no longer copied out of archives.
- **Single-instance launcher.** The launcher refuses to open a second copy (two launchers could start two
  games fighting over the same saves); a hidden `--allow-multiple` flag overrides it for development.
- **Clearer update errors + window focus.** When a release has no Linux package yet, the updater explains that
  instead of a bare "Archive download failed"; the launcher window now raises/activates itself on show.

_Engine is unchanged from 5320 — this is a launcher-focused release._

## 1.4.0.5319 — 2026-07-27
- **Weekly upstream sync (manual merge).** Folded in the KeeperFX team's latest 9 patches (#5039–#5062).
  The auto-sync bot correctly flagged a merge conflict in `engine_render.c` for human resolution rather than
  forcing a bad merge; resolved by hand and build-verified on Linux. Highlights:
  - **Rendering fixes** — big sprites no longer clipped by floor tiles in straight view (#5062); the Dungeon
    Heart no longer clips when zoomed in straight view (#5060); the landview (map) overlay no longer drops an
    edge pixel at high resolution (#5058). All three benefit our high-res Linux setups.
  - **UTF-8 text input** (#5039): accented characters (ñ, ç, é, ¿¡ …) can now be typed in save names, chat,
    and other text fields.
  - **Mapmaker/modding:** new `COPY_CREATURE_TYPE` script command (#5050); maps can load string subtypes in
    `.tngfx` files (#5052).
- **Converged with upstream on the poly-pool clear.** Our 5305 Linux perf pass had trimmed the renderer's
  per-frame 16 MB poly-pool `memset` down to just the high-water-mark region; upstream's #5047 ("Memset
  optimization") then removed it **entirely** — the fuller form of the very insight our own analysis reached
  (the bucket-heads clear is what guarantees correctness; the pool clear was always redundant). We adopted
  upstream's version and dropped our now-superseded code. Same result, one source of truth, and this hot file
  no longer re-conflicts on every sync.

## 1.4.0.5305 — 2026-07-20
- **Linux engine performance pass.** Four output-identical optimizations to the native Linux build,
  found by a multi-agent performance audit and verified in-game across multiple levels:
  - **Renderer no longer zeroes a 16 MB buffer every frame.** The isometric view cleared its entire
    polygon pool each frame regardless of use; it now clears only the region actually touched.
  - **The GPU no longer re-uploads the palette every frame.** The OpenGL present path re-sent the
    256-colour palette (CPU expand + texture upload + driver sync) every frame; it's now guarded to
    upload only when the palette actually changes (fades/flashes/movies still work).
  - **No per-turn CPU busy-spin.** The turn pacer busy-spun the tail of every game turn (a leftover
    Windows timer workaround); the Linux path now sleeps precisely with a high-resolution timer,
    lowering idle CPU and helping laptop battery/thermals.
  - **Faster sprite/text blit.** The core sprite/glyph copy is now a single (SIMD-vectorized) `memcpy`
    instead of a byte-at-a-time loop.
- *A wider `-march=x86-64-v2` build was tried and reverted:* GCC's auto-vectorizer miscompiled a loop in
  the 25-year-old pathfinding code, crashing on level start. Correctness first — the four wins above are
  independent of it.

## 1.4.0.5296 — 2026-07-20
- **Merged the KeeperFX team's latest alpha patches** (weekly upstream-sync bot, 19 commits,
  #5019–#5049). Verified to build clean on Linux before release. Highlights:
  - Engine performance: `gpoly` polygon rasterizer optimized for ~10% faster rendering (#5043)
    and map-parchment (minimap) redraw performance fixed (#5041) — both benefit single-player
  - New feature: **map-specific sounds and speeches** — maps can ship custom sounds/speech in
    their `.zip` bundle (#5019), backed by a new shared `custom_zip` reader factored out of the
    sprite loader
  - Gameplay/engine fixes: menu-sound gating fixed (#5028), mouse-light flicker on GUI menus
    fixed (#5040), highscore input caret scaled/shaded/framed correctly (#5044), computer
    players no longer sell enemy traps when building rooms (#5034), `pngpal2raw` tool build
    error fixed (#5035)
  - Maps: **Fortress** added, Bash neutered, Dread Mountain fixed (#5032)
  - Controller rumble now only fires when a controller was the last input used (#5049)
  - Multiplayer: reliable slap/power-click (#5026), version-mismatch message on start (#5025),
    client clock synced to host packet-receive time (#5031), dynamic input-lag adjustment
    (#5033, #5036), desync fixes + logging (#5042), possible ghost-tagging fix (#5030), fixed
    picking up and casting at the same time (#5038), client-side info buttons (#5045)

## 1.4.0.5273 — 2026-07-15
- **Fixed: game config was frozen in the update payload.** The release pipeline built the
  `full.7z`/AppImage payload from the previous release and only refreshed the engine binary,
  fonts, and launcher — never the game config (`fxdata`/`creatrs`/`mods`). So config changes
  never reached players even as the engine advanced. This shipped a stale `objects.cfg` in
  5272 that **broke slapping chickens** (the new engine gates object-slapping on a `SLAPPABLE`
  flag the frozen config lacked) and, via the mismatched hand/object code, caused a
  **heap-corruption crash on the level-victory transition**. The packaging now overlays the
  freshly-built engine config onto the payload, matching the portable-tarball build. Restores
  chicken-slapping and fixes the crash; no engine code change.

## 1.4.0.5272 — 2026-07-15
- **Merged the KeeperFX team's latest alpha patches** (weekly upstream-sync bot, 22 commits).
  Highlights:
  - Engine performance: path-finding map generation optimized and split out into
    `ariadne_update.c` (#5012), the ariadne navigation module refactored (#5004), and room
    recalculations optimized (#5002)
  - Camera/input: rotate the camera around the mouse position (#4898), Delete/PageDown default
    to rotate/zoom again (#5010), fixed hand-of-evil animation issues (#5011)
  - Gameplay fixes: magic boxes no longer unclickable (#5001), GUI pickup no longer slightly
    slow (#5009), fixed double sound fade-out on the landview ensign (#4993), campaign creature
    lists now replace rather than append (#4999)
  - Lua: new OnPickup and OnSlap triggers (#4805), fixed ACTIVATIONLUAFUNC (#5015)
  - Continues are now compatible between future versions (#5014)
  - Translated error messages across all languages (#4997)
  - Multiplayer: local-camera tagging (#5018), resync console command (#5005) + lua-data resync
    (#4966), reliable creature clicking (#5000), stutter tracking (#5006), hand-of-evil fixes
    (#5003), clean disconnect after failed startup (#5008), FRAMES_PER_SECOND limit re-enabled
    (#4994)

## 1.4.0.5246 — 2026-07-08
- **Merged the KeeperFX team's latest alpha patches** (first pull request from the weekly
  upstream-sync bot). Upstream bumped the base version to **1.4.0**. Highlights:
  - Improved frame interpolation (#4919) and fixed/improved fast-forward frame skip (#4989)
  - Multiplayer: keep lobbies alive after going to the landview screen (#4981), close
    lobbies faster (#4982), fixed network crash 449 (#4992), fixed workshop-item bounding
    boxes turning invisible (#4991)
  - Two new 2v2 multiplayer maps (#4987)
  - Slappable objects (#4894)
  - Lua: creature transform function (#4985) and max level for new creatures (#4978)
  - Music loading for mods (#4952)
  - Fixed SDL compile race / library targets (#4983, #4976), missing return in
    SET_BOX_TOOLTIP (#4979), removed unused Name fields in configs (#4980), in-game error
    message for misplaced zip files (#4995)

## 1.3.2.5224 — 2026-07-01
- **Merged the KeeperFX team's latest alpha patches:**
  - Native Linux build works on modern GCC (#4962)
  - Fix lobby chat (#4967) and disable unsynced multiplayer lua date/time functions (#4958)
  - Legacy Disease hurt time 400→144 (#4968)
  - Actionable things flash under the cursor again when using cheats (#4977)
  - Fix modded creatures refusing to train (#4975)

## 1.3.2.5216 — 2026-06-30
- **Case-insensitive paths now resolve full directories,** not just the filename — so a
  mod/campaign that references e.g. `mods/MyMod/sound/Foo.wav` still loads when the real
  folders are lower-case. (Exact-case paths are unchanged — purely a fallback.)
- **Crash handler is async-signal-safe:** it no longer tears down the screen/GL/SDL from
  inside the signal handler (that could deadlock when the fault was in the GL driver or the
  allocator and suppress the crash report); the signal info + backtrace are still written.

## 1.3.2.5215 — 2026-06-30
- **More audit fixes (correctness & robustness).** Following the security pass: fixed
  several `ScriptValue` union-aliasing bugs (wrong objective/information popup location,
  wrong `move_creature` target, wrong `add_object` angle), added missing range-check
  returns in `USE_POWER_ON_CREATURE`, corrected `%I64d` formats and a `(bpp = 32)`
  assignment bug, hardened slabset config parsing, and stopped a malformed WAV from
  hanging the loader.
- **Audio works on case-sensitive filesystems.** Sound banks, music, streamed samples and
  movies now resolve the real on-disk filename case, so mixed-case files referenced by
  mods/campaigns play on Linux instead of silently failing.
- **Launcher: Install… no longer overwrites stock files.** Installing a campaign/mod/map
  pack keeps existing files (stock campaigns, other add-ons) instead of clobbering them.

## 1.3.2.5213 — 2026-06-30
- **Security/stability — hardened against crafted add-ons:** a multi-agent Linux audit
  found several out-of-bounds writes a malicious or buggy workshop campaign/map/mod could
  trigger. Fixed: `SET_BOX_TOOLTIP` and `SET_POWER_CONFIGURATION` script-index checks;
  `.lif`/`.slx`/texture/palette file loads now bound the read to the buffer (RNC-unpacked
  size); PNG sprite RLE buffer sizing; and the CPU movie path skips oversized frames.
  Reported upstream (dkfans/keeperfx#4970).
- **Launcher:** the Map Editor (Unearth) and the clickable workshop/news tiles now launch
  with a clean environment, so they work from the AppImage like the other buttons.

## 1.3.2.5211 — 2026-06-30
- **Fixed a campaign crash (segfault):** the `QUICK_MESSAGE` and `DISPLAY_MESSAGE`
  script commands corrupted the message index when given a chat-icon/player argument
  (a `ScriptValue` union overlap), causing an out-of-bounds read on level start. This
  crashed workshop campaigns such as **Tempest Keeper** and **Another Dungeon** the
  moment their level script ran. Reported upstream (dkfans/keeperfx #4969).

## 1.3.2.5209 — 2026-06-30
- **Universal "Install…" in the Mod Manager:** one button now installs any KeeperFX
  add-on from a `.7z`/`.zip` — mods, **campaigns**, and map packs — putting each in
  the right place (campaigns appear in the in-game Land selection). Replaces the old
  mod-only "Install mod…", which mis-filed campaigns as mods.
- **Fixed the Workshop / Discord / Website / Open-folder / Open-log buttons** doing
  nothing under the AppImage: they now launch your browser/file manager with a clean
  environment instead of the bundled one.
- **Faster release CI:** AppImage builds are cached (ccache + Qt + deps); the Flatpak
  moved to its own monthly/manual workflow.

## 1.3.2.5208 — 2026-06-30
- **Install mods from the launcher:** the Mod Manager has an **Install mod…**
  button — pick a `.7z`/`.zip` (e.g. a keeperfx.net workshop download) and it
  extracts it into your `mods/` folder, generating a `mod.cfg` if the archive
  lacks one, then refreshes the list. No more hand-extracting into the install dir.
- **Launcher size — "Comfortable (110%)":** a smaller step between Normal (100%)
  and Large (125%), so the UI scale jump isn't so big.
- **Mod thumbnails:** mods without their own picture now show the KeeperFX icon as
  a placeholder instead of an empty frame.

## 1.3.2.5207 — 2026-06-30
- **In-launcher Mod Manager:** the launcher now lists the mods in your install's
  `mods/` folder with an Enabled toggle that reads and rewrites `load_order.cfg`,
  so you can turn mods on/off without hand-editing the file. (Section grouping and
  drag-to-reorder are planned next.)
- **Launcher updates ship in `full.7z`:** the AppImage CI now packages the same
  flat payload it builds — including the freshly-built launcher — as the update
  archive, so launcher fixes reach existing installs through **Update**, not only
  through a fresh AppImage download.

## 1.3.2.5206 — 2026-06-29
- **Accessibility — "Launcher size":** a new Settings → Launcher dropdown scales
  the whole launcher UI (Normal / Large / Larger / Huge / Largest, 100–200%) for
  players with low vision. Persists, and applies on the next launch.

## 1.3.2.5205 — 2026-06-28
- **Upstream alpha patches merged** (KeeperFX `master`, patches 5177–5179):
  - Fix tooltip boxes being too big again (#4959)
  - Fixed creatures not colliding with other things (#4955)
  - Fixed swap creature on `map.creature` again (#4960)

## 1.3.2.5200 — 2026-06-28
- **Self-updating launcher:** the launcher is now part of the update package, so
  in-launcher updates refresh the **launcher itself** — not just the engine and
  game data. Earlier builds only updated the engine, so launcher fixes (like the
  hidden download-server picker) needed a fresh AppImage download. The AppImage no
  longer overwrites a launcher that an update has upgraded.
- **Update package is flat:** the `full.7z` now unpacks files at the install root
  instead of a nested folder, fixing an update *loop* where the version never
  "stuck" and the launcher re-downloaded the same release on every launch.

## 1.3.2.5199 — 2026-06-28
- **Distribution:** one-file **AppImage** and **Flatpak** for any current 64-bit
  Linux distro (built on Ubuntu 24.04 via CI, runs everywhere — Ubuntu, Fedora,
  Arch, Steam Deck). Each bundles the engine, every library, and the game data;
  the only thing you provide is your own *Dungeon Keeper* files.
- **Launcher (Linux-native):** runs the engine directly (no Wine); reads the real
  version; installs/updates from this repo's GitHub releases; adds an applications
  **menu entry** on first run.
- **DK auto-detect:** the launcher now *searches* the disk (Wine/Lutris/Heroic/
  Steam/GOG/mounts) for your Dungeon Keeper files, instead of only checking fixed
  paths — and tells you clearly if it can't find them.
- **In-launcher updates:** "Update available — vX" / "No updates at this time"
  against this repo's releases.
- **UI tidy:** hide the keeperfx.net/Cloudflare download-server picker (we use
  GitHub); quiet harmless AppImage log noise (Wayland plugin / gvfs module).
- **Engine extras (carried from earlier):** GPU OpenGL-3.3 display path, truecolor
  front-end movies, and fixes for clean exit, ultrawide creature-possession, and
  the UTF-8 font crash.

See the full project description in [README.md](README.md). The truecolor/Vulkan
3D renderer remains **work in progress** (not enabled).
