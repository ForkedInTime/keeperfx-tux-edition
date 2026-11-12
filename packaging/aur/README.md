# AUR packaging

Source for the `keeperfx-tux` AUR package. This directory is the canonical copy;
the AUR repository is a mirror of these four files plus a generated `.SRCINFO`.

The package is **versioned against release tags**, not a `-git` package: it
builds whatever `_tag` points at, so `yay -Syu` shows an update like any other
package instead of requiring `--devel`. The cost is one bump per release, which
`.github/workflows/publish-aur.yml` does automatically.

## One recipe, three packages

It is a **split package**: a single `pkgbase` in a single AUR repository that
produces three outputs.

| Output | Source | Size | Contents |
|---|---|---|---|
| `keeperfx-tux` | this repo at `_tag` | 12 MB | engine compiled from source, text config, `multiplayer/`, wrapper, desktop entry, icons |
| `keeperfx-tux-data` | the release's `full.7z` | 393 MB | `data` `sound` `ldata` `campgns` `levels` `lang` `fxdata` `creatrs` `mods` `music` `multiplayer` |
| `keeperfx-tux-launcher` | the launcher repo at `_launcher_commit` | 2 MB | Qt launcher, its wrapper, desktop entry, icon |

Users run `yay -S keeperfx-tux` and pacman pulls the other two in as
dependencies, so there is one name to remember. The launcher is a hard
dependency rather than optional because it is what finds a Dungeon Keeper
installation and copies the files in — without it a new player has a game they
cannot set up.

**Two menu entries, deliberately.** `KeeperFX` runs the launcher (the front
door: find, install, configure, play) and `KeeperFX (play directly)` runs the
engine, for once the game is set up. That mirrors the AppImage, whose entry
point is also the launcher.

**The launcher is copied into the game directory, not symlinked.** It resolves
`keeperfx.cfg` and the game binary against
`QCoreApplication::applicationDirPath()`, and Qt reads `/proc/self/exe` — so a
symlink reports the *target's* directory, not the game directory. The engine's
`argv[0]` trick does not transfer. Its wrapper copies the binary in and
refreshes it when the packaged build differs, exactly as the AppImage's AppRun
does.

**Do not try to satisfy the launcher's dependencies from the system.**
`CPM_USE_LOCAL_PACKAGES` was tried: it fails on LIEF, because `lief-bin` ships
only the shared library while this build asks for `COMPONENTS STATIC`, and it
silently satisfied `bit7z` from an unpackaged `/usr/local` copy on the build
machine — which would have produced a package that builds nowhere else. CPM
fetches both, and `CMAKE_SKIP_RPATH` keeps the in-tree zlib build directory out
of the shipped binary. Building both from one `pkgbase` also makes a
version mismatch between engine and data impossible by construction rather than
something CI has to police — and a mismatch is precisely the staleness that once
shipped a frozen config against a newer engine.

Since 1.4.0.5273 the release archive carries freshly-overlaid config — verified for
1.4.0.5366, where all 65 `fxdata`/`creatrs` files are byte-identical to the repo at
that tag — so the data package's copies are authoritative and the wrapper prefers
them.

The data package deliberately drops the archive's `keeperfx` binary, the Qt
launcher, the bundled `7z.so`, `keeperfx.cfg` and `version.txt`: those are either
the engine package's job or user state.

**Redistribution note.** The archive contains no files from the original Dungeon
Keeper except `data/main.pal` and `data/mapfadeg.dat`, which do appear in
`docs/files_required_from_original_dk.txt`. They arrive via the KeeperFX data tree
that upstream distributes, and the AUR hosts nothing itself — it points users at
the same release asset this project already publishes — so this package adds no
exposure that the AppImage and `full.7z` do not already carry. The other 14 listed
files are absent, and the wrapper names them individually when they are missing.

## What the engine package does and does not ship

Ships: the engine built from source against system libraries, the generated
UTF-8 fonts, and the text/config data that tracks the engine version
(`fxdata`, `creatrs`, `mods`, `campgns`, `levels`, `lang`) — enough to run
against a data tree the user already has.

Does not ship: the bulk game data, which is what `keeperfx-tux-data` is for, nor
the 14 files listed in `docs/files_required_from_original_dk.txt`, which come
from the user's own copy of Dungeon Keeper and are in no package at all.

## Why there is a wrapper instead of a plain binary

The engine derives every path it touches from the directory of `argv[0]` —
see `process_command_line()` in `src/main.cpp`, which takes `argv[0]`, strips
the last component, and stores the result as `keeper_runtime_directory`. Save
games (`FGrp_Save`), screenshots, `keeperfx.cfg` and `keeperfx.log` all resolve
against that directory, so a read-only `/usr` prefix cannot serve as the
runtime directory.

`/usr/bin/keeperfx-tux` therefore assembles a per-user game directory at
`$XDG_DATA_HOME/keeperfx-alpha` (override with `KEEPERFX_HOME`) and execs the
engine through a path inside it. Because the engine reads `argv[0]` rather than
`/proc/self/exe`, the symlink is not resolved and the engine roots itself in the
writable directory.

The game directory stays `keeperfx-alpha` rather than following the package
rename: that is the path `refresh-alpha.sh` and the Qt launcher already use, and
changing it would strand existing installs and stop the launcher from finding
data it had fetched.

Layout the wrapper maintains, resolving each tree against the data package first
and the engine package second:

| Kind | Directories | Behaviour |
|---|---|---|
| Merged | `lang` `fxdata` `creatrs` `ldata` `multiplayer` `data` `sound` `campgns` `levels` | Every directory, at any depth, is a real one owned by the user; only the packaged **files** are links, refreshed each launch so a pacman upgrade takes effect, and a link to a file the package no longer ships is removed. So the user's own Dungeon Keeper files and add-ons sit beside the packaged ones anywhere: a Workshop map that adds itself to `levels/standard`, a campaign, multiplayer maps. `levels/personal` and `levels/legacy` hold copies, not links, of the readme the package puts there |
| User-owned | `mods` `music` | Seeded once with `cp -rn`, never clobbered |
| Writable state | `save` `scrshots` | Created if absent |

The packaged game always wins where it ships something. The AppImage deploys its
whole payload into this same directory as real files, so anyone who ran it before
installing the package has a copy of every packaged file in the way. A copy that
is byte-identical to the packaged file, or to one already moved aside, is replaced
by the link. Anything else is moved to `.replaced-by-package/<date-time>/` in the
game directory, never deleted, with a README.txt saying why, and the wrapper says
so on stderr. If a copy cannot be moved (a directory the user cannot write to),
the game still starts and keeps using it. The user's own Dungeon Keeper files are
never in the way, because the package ships none of them.

## Maintenance

**Per release: nothing.** Publishing a GitHub release triggers
`.github/workflows/publish-aur.yml`, which rewrites `_tag`/`pkgver`, regenerates
`.SRCINFO`, commits the bump back here, and pushes to the AUR. It needs one
repository secret, `AUR_SSH_PRIVATE_KEY`, whose public half is registered on the
AUR account owning the package. Run it with **Actions > Publish to AUR > Run
workflow** and `dry_run: true` to validate without pushing.

**When Arch updates a library the engine links.** Everyone compiles the package
against the libraries Arch has that day, and its `depends=` holds it to them, so
when Arch moves one to a new soname (ffmpeg 8 to 9 did) pacman refuses the library
upgrade, and `yay -Syu` stops, on every machine with the game installed, until the
package is rebuilt. `.github/workflows/aur-library-watch.yml` compares Arch with
`packaging/ci/aur-library-baseline.txt` every Wednesday. On a change it test-builds
the live AUR recipe against current Arch and opens an `aur-libraries` issue. To
rebuild: **Publish to AUR > Run workflow** from the stable's branch with its tag and
`rebuild` ticked (only `pkgrel` moves, above the AUR's, and the launcher pin is
kept); then run **Watch Arch libraries** with `accept` to record the new baseline.
The stable's branch needs a `publish-aur.yml` that has the `rebuild` option: one cut
before it existed (1.4.0.5660, `hotfix/1.4.0.5660`) needs it committed there first.
Run `accept` after every new stable is published too.

**When you change the wrapper or desktop file**, their checksums are pinned, so
refresh them:

```bash
updpkgsums
makepkg --printsrcinfo > .SRCINFO
```

**First publish** (one-off — the AUR creates a package repo on first push, which
CI cannot do because it clones an existing one):

```bash
./import-to-aur.sh
```

Building locally to test a change:

```bash
makepkg -f                       # build only
KEEPERFX_HOME=~/kfx-aur-test makepkg -si   # build, install, then run keeperfx-tux
```

Use `KEEPERFX_HOME` when testing on a machine that already has a
`refresh-alpha.sh` install: the wrapper deliberately refuses to overwrite real
directories, so it would silently keep using the existing game directory instead
of the packaged data.

## Deliberate choices a reviewer may query

- **`prepare()` reaches the network.** `linux.mk` fetches centijson, astronomy
  and enet6 rather than vendoring them (libcurl is Arch's: `SYSTEM_CURL=1`). Doing it in `prepare()` at least
  keeps `build()` offline-shaped. Vendoring them as additional `source=()`
  entries would be cleaner and is the obvious follow-up.
- **`!strip`.** This is an alpha; usable backtraces in bug reports are worth
  more than the saved megabytes.
- **`!lto`.** The makefile manages its own optimisation flags, and LTO has not
  been validated for this codebase. Letting makepkg inject it risks a silent
  miscompile of the kind that already forced a `-march` revert.

## Verification status

Done:

- **Clean-chroot build** (`extra-x86_64-build`) of the engine package: succeeds
  from `depends`/`makedepends` alone, so nothing leaks in from the build machine.
- **namcap**: no errors. Five warnings remain, all understood — `!strip` is
  deliberate, FULL RELRO is absent because `linux.mk` sets its own `LDFLAGS`
  rather than honouring makepkg's, and the `gcc-libs`/`libstdc++`/`libgcc`
  triplet is namcap contradicting itself about a dependency that is correct.
- **Data output**: 412 MB, every expected tree, no engine binary or launcher,
  none of the 14 restricted original-DK files.
- **End to end**: both outputs installed with `pacman -U` on an Arch desktop and
  the game reached "Started level 1 from Dungeon Keeper original campaign" on the
  OpenGL 3.3 backend, using a real Dungeon Keeper copy for the 14 files.
- **Wrapper**: game-directory assembly, idempotency, preservation of user-edited
  mods and of a real directory replacing a symlink, a user file overriding a
  packaged one, the engine-only fallback, the missing-originals message, and
  `argv[0]` rooting in the game directory while the ELF lives in `/usr/lib`.

Not done: **launching the packaged game end-to-end**, which needs a real
`data/` + `sound/` including the user's own Dungeon Keeper files. Install with
`KEEPERFX_HOME=~/kfx-aur-test` and play a level to close this out.
