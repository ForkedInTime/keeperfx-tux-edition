# Building and maintaining the Tux Edition

<sub>[← Back to the README](../README.md)</sub>

## Build from source

Tested on Arch Linux (x86-64). Package **names** differ across distros, but the set is the same.

> **A source checkout is not a playable game, by design.** The repository tracks the engine and the
> campaigns' *scripts and configuration* — `campgns/keeporig` is 26 `.txt` files here, and 315 files
> in a release, the difference being the binary map data (`.slb`, `.own`, `.tng`, `.wib`, `.lgt`).
> That content comes from the upstream release payload during release assembly, so building from
> source gives you an engine, and you still need a release (or the launcher) for something to play.

**1. Install build dependencies** (Arch / derivatives):
```bash
sudo pacman -S --needed base-devel git python \
  sdl3 sdl3_mixer sdl3_image \
  ffmpeg openal luajit libspng minizip zlib \
  miniupnpc libnatpmp curl
```
> Other distros: a C/C++ toolchain, `make`, `git`, `python3`, and the dev packages for SDL3 (+mixer/image),
> ffmpeg (avformat/avcodec/avutil/swscale/swresample), OpenAL, LuaJIT, libspng, minizip, zlib,
> miniupnpc, libnatpmp, libcurl. centijson/astronomy/enet6 are fetched by the makefile.

**2. Clone and build the engine:**
```bash
git clone https://github.com/ForkedInTime/keeperfx-tux-edition.git
cd keeperfx-tux-edition
make -f linux.mk \
  deps/centijson/include/json.h deps/astronomy/include/astronomy.h \
  deps/enet6/include/enet6/enet.h
rm -f src/ver_defs.h
make -f linux.mk SYSTEM_CURL=1 BUILD_NUMBER="$(packaging/linux-only/build-number.sh)" VER_SUFFIX=alpha -j"$(nproc)"
```
This produces **`bin/keeperfx`** (a native ELF). `SYSTEM_CURL=1` links your system's libcurl, as the AUR
package does. Without it the makefile uses the prebuilt static libcurl the release builds bundle (add
`deps/libcurl/lib/libcurl.a` to the first command, plus the openssl and zstd dev packages); that one is
built against OpenSSL 3 and does not link where the system has OpenSSL 4, as Arch has since November 2026.
Switching between the two in one tree: delete `obj/` first, so nothing stays compiled against the other's headers.

**3. Generate the UTF-8 fonts** (the makefile doesn't build them):
```bash
packaging/ci/make-fonts.sh    # writes tools/fxfontmaker/*.fxfont, the same script every release uses
```

**4. Assemble a playable install.** This repo has engine source + text config only — not the game data. Get a
KeeperFX data tree (via the launcher or an official install), then overlay your build + fonts + `config/` and
write `version.txt`. The whole build-and-deploy is automated by **`./refresh-alpha.sh`**.

The native launcher is its own repo:
[**keeperfx-launcher-qt-linux**](https://github.com/ForkedInTime/keeperfx-launcher-qt-linux).

## 🔧 Why this fork builds with `linux.mk` only

Upstream builds with CMake, and in August 2026 deleted its own `linux.mk` (#5083); this fork kept
`linux.mk` and does not carry the CMake build. CMake was never what CI, the AppImage or the AUR package
were built with. Upstream's CI does compile it natively on Linux for its pull-request test builds, but its
presets are MinGW, MSVC and clang-cl plus a Steam Deck developer bundle driven from a separate PC, its
Windows dependencies come from vcpkg, and on this fork's tree — the last time it was measured — it stopped
at the link step for want of `libswscale`, which this fork's movie player (`bflib_fmvids.cpp`) needs and
upstream's does not.

So the Linux-only filter removes the whole CMake tree, with vcpkg, the MinGW toolchain and the Deck bundle
([`packaging/linux-only/remove.list`](../packaging/linux-only/remove.list)). `linux.mk` is the build — CI, the
AppImage, the Flatpak and the AUR package all use it — and the weekly sync now keeps its source list current
by itself: sources the team adds or deletes are added to or dropped from it in the sync's own merge commit.

None of that is a criticism of upstream: KeeperFX is a Windows project and their CMake serves their
platform. If it ever becomes the better way to build this edition, bringing it back is one deleted line in
`remove.list` — the next snapshot then includes it.

## How the Tux Edition stays current with upstream

**A weekly sync bot does it automatically.** Every Monday a GitHub Action checks the KeeperFX team's
`master` for new commits, takes a **Linux-only snapshot** of it — their tree with every Windows-only file
and `#ifdef _WIN32` branch removed — and merges that:

- **nothing new, or only Windows-only changes** → exits quietly;
- **merges cleanly, compiles, passes the tests and the filter proof** → opens a pull request into `alpha`
  listing every upstream change, with a warning at the top if it would stop existing saved games from
  loading. Nothing merges itself: I read the diff before clicking **Merge**;
- **merge conflicts, a red build or a failed check** → opens an issue for manual attention instead (or adds
  the week's result to the one already open).

A team fix wanted sooner is taken ahead of the sync, with its author. Every day a check then compares each
of those with the team's later commits to the same code and fails a release that lacks a later fix it
needs ([`docs/releasing.md`](releasing.md#team-commits-taken-early-and-the-teams-fixes-to-them)).

### Curation policy, how releases are cut, and manual syncing

**We track upstream, but on our terms — we curate, we don't blindly fast-forward.** The bot automates the
routine merges; anything that conflicts (usually where our Linux-specific changes overlap a hot engine file)
is resolved by hand, at our discretion. We take upstream's improvements — *including when they supersede our
own*: e.g. their #5047 "Memset optimization" removed a wasteful per-frame buffer clear that we'd already
trimmed independently, so we adopted theirs and dropped ours — and we keep our Linux layer where it's the
better fit. The result is a fork that stays current with the team's work while remaining a deliberate,
reviewed selection rather than an automatic mirror.

**The Windows half doesn't come along for the ride — not the plumbing, not the code.** Upstream builds for
Windows first: a Visual Studio project, MinGW and MSVC CMake presets, vcpkg, Windows CI that signs `.exe`
patches, a Windows debugger binary in `.vscode/`, and `#ifdef _WIN32` branches through the engine. None of it
is in this repository. The sync never merges the team's commits directly: it filters their tree first —
deleting the files in [`packaging/linux-only/remove.list`](../packaging/linux-only/remove.list) and, with
[unifdef](https://dotat.at/prog/unifdef/), every preprocessor branch that only a Windows compiler would ever
take — and merges that snapshot. Because the previous snapshot is the merge base, Windows content is on
neither side of the merge: it can't conflict and can't come back, so stripping it costs nothing week to
week. The filter is **proven** not to change the game: every source the Linux build compiles is
preprocessed with and without it, and the results must be identical. A guard workflow fails any pull request
that reintroduces Windows content. The full story, including how to sync by hand, is in
[`packaging/linux-only/README.md`](../packaging/linux-only/README.md).

After the sync PR is merged, a release is cut, and publishing it is the whole job: CI builds the AppImage,
the game package (`full.7z`), the portable tarball and a small update patch from the previous release of
each channel, and attaches them all to the release; a stable release also updates the AUR recipe. Each
build layers the KeeperFX team's current data package over ours, so their new content arrives on its own
rather than waiting to be noticed, and the artwork itself is regenerated from the team's FXGraphics repository on every build, and the game's text from their translations, so sprites and messages that arrive alongside new code arrive with it. The Flatpak is rebuilt on the 1st of each month (or by hand) and
attached to the newest release of that day; in between, the launcher inside it offers new game releases
like any other install. Existing installs are offered the new build by the launcher's built-in updater,
which downloads the patch when one fits and the full package otherwise.

The same sync can still be done by hand (needs `unifdef`: `sudo pacman -S unifdef`):
```bash
git fetch upstream                                                  # the KeeperFX team's master
git log --oneline "$(packaging/linux-only/synced-upstream.sh)..upstream/master"   # what they changed
git merge "$(packaging/linux-only/make-snapshot.sh upstream/master)" # merge the Linux-only snapshot
packaging/linux-only/sync-linux-mk-sources.sh                      # add their new sources to linux.mk
./refresh-alpha.sh                                                  # rebuild and redeploy locally
```
Never `git merge upstream/master` directly: that brings the Windows content back.
