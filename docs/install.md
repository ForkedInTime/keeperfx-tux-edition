# Installing KeeperFX — Tux Edition

<sub>[← Back to the README](../README.md)</sub>

- [AppImage](#appimage--any-current-64-bit-distro) — one file, any current 64-bit distro
- [Arch Linux (AUR)](#-arch-linux--install-from-the-aur) — a real package, updated with `yay -Syu`
- [Flatpak](#flatpak--or-the-appimage)
- [The complete package](#the-complete-package-manual-install) — by hand, with your own libraries
- [System requirements](#system-requirements)

## AppImage — any current 64-bit distro

Download **[KeeperFX-Tux-Edition-x86_64.AppImage](https://github.com/ForkedInTime/keeperfx-tux-edition/releases/latest/download/KeeperFX-Tux-Edition-x86_64.AppImage)**
(~500 MB). That link is always the newest **stable** build; alphas and older builds are on
[all releases](https://github.com/ForkedInTime/keeperfx-tux-edition/releases). Then:

```bash
chmod +x KeeperFX-Tux-Edition-x86_64.AppImage
./KeeperFX-Tux-Edition-x86_64.AppImage
```

That **one file** has everything — launcher, engine, every library, and the game data. Nothing to `apt install`.
It opens the settings launcher, finds your *Dungeon Keeper* install, lets you set up graphics/sound, and plays.

🔑 **The only thing you provide is your own original *Dungeon Keeper* files** — from
[GOG](https://www.gog.com/game/dungeon_keeper),
[Steam](https://store.steampowered.com/app/1996630/Dungeon_Keeper_Gold/),
[EA](https://www.ea.com/games/dungeon-keeper/dungeon-keeper), or an old CD. The launcher copies them for you.
(We never ship the original game — you must own it.)

> 🐧 **No GOG Galaxy on Linux?** There's no native GOG client — so see
> **[How do I install DK1 on Linux?](faq.md#how-do-i-install-dk1-on-linux)** for getting your GOG/Steam/EA
> copy installed (via Lutris or Heroic) so the launcher can find it.

Runs on any current 64-bit distro — Ubuntu 24.04 / 26.x, Debian 13, Fedora, Arch, Steam Deck, … &nbsp;
*(not Ubuntu 22.04 or older — see [requirements](#system-requirements)).*

### Won't start? ("Cannot mount AppImage, please check your FUSE setup")

This AppImage uses the current static runtime, so it does **not** need the old `libfuse2` package that
many AppImage guides tell you to install — it mounts itself on a stock Ubuntu 24.04, verified. The one
thing it needs is FUSE in the kernel (`/dev/fuse`), which every desktop install has. If you are somewhere
without it — a container, a locked-down kernel — run it without mounting instead:
`./KeeperFX-Tux-Edition-x86_64.AppImage --appimage-extract-and-run`

That's the AppImage *runtime*, not our app — everything our game needs is already bundled.

## 🏛 Arch Linux — install from the AUR

**Arch, CachyOS, EndeavourOS, Manjaro:** there's a proper package —
[**keeperfx-tux** on the AUR](https://aur.archlinux.org/packages/keeperfx-tux). No blob to download, no
`chmod +x` — the engine is compiled from source against your system's own libraries.

```bash
yay -S keeperfx-tux
```

That one command is the whole install — **the launcher and game data come with it**, not as separate
steps: `keeperfx-tux` pulls in `keeperfx-tux-data` (campaigns, graphics, sounds) and
`keeperfx-tux-launcher` (the Qt launcher) automatically, all locked to the same version.

**First launch: open “KeeperFX” from your menu — that's the launcher, and it finishes the setup.**
It finds your *Dungeon Keeper* installation (GOG, Steam, EA, Lutris/Heroic) and copies the handful of
original files the packages legally can't ship, fetches the soundtrack if your copy lacks it, and is
also where the Workshop browser and Mod Manager live — one-click campaigns, maps and mods. The second
menu entry, **“KeeperFX (play directly)”**, skips the launcher and starts the game itself: fine for
every day *after* setup, but on a fresh install it will simply stop and list the files it's missing.

Updates arrive with your normal `yay -Syu` — the AUR tracks this project's **stable** releases, so an
upgrade only ever moves you between stables, never onto an alpha. Because the engine is built against
your system's libraries, an Arch library update can leave it needing a rebuild, and `yay -Syu` would
stop until it gets one; a weekly check compares Arch with what the current stable was built against
and flags such a change, so the rebuild (only the package release number moves) can go out. Prefer to
build without an AUR helper? `git clone` this repo and `makepkg -si` in `packaging/aur/` produces the
identical packages — same recipe.

### What the package does and doesn't include

It installs as three packages from one recipe: `keeperfx-tux`, the engine, compiled against your
system's SDL3/ffmpeg/OpenAL rather than bundling copies; `keeperfx-tux-data`, everything KeeperFX
itself provides — campaigns, graphics, sounds and language files; and `keeperfx-tux-launcher`, the Qt
launcher, pinned to the same launcher commit the AppImage of that release was built from. You only
ever ask for the first; pacman brings the other two, and an upgrade keeps the previous engine so
your saves stay playable.

On first launch the game directory is assembled at `~/.local/share/keeperfx-alpha` (set `KEEPERFX_HOME`
to put it elsewhere). Every folder in it is yours; only the game's own files inside are links to the
package, so Workshop maps, campaigns and mods install anywhere in it and a pacman upgrade takes effect on
the next start. If an AppImage ran there before, the package replaces the copies of its files that
the AppImage left: identical ones become links, anything else is moved to `.replaced-by-package/` in that
folder, never deleted. The folder is named after this project's first name and keeps it, stable and
alpha alike: it is where every install keeps its saves. **You still need to own the original game**,
same as every other install method here: 14 files from your *Dungeon Keeper* CD or GOG/Steam/EA copy are
not redistributable, so they are not in any package. If they are missing the game names them exactly
and stops, and the Qt launcher can find an installation and copy them for you.

The recipe lives in `packaging/aur/` in this repository, so what you build with `makepkg` above is
exactly what will be published to the AUR — same PKGBUILD, same three packages, same paths.

## Flatpak — or the AppImage?

Both contain the same game; they're just two ways to install it.

- **AppImage — recommended.** The simplest: download one file, make it runnable, go. Nothing is installed
  into your system, and you can delete it any time. Best if you just want to play.
- **Flatpak.** Pick this if you already use Flatpak and want KeeperFX in your app menu with automatic
  updates. It needs Flatpak set up first and downloads a shared system runtime the first time (a few hundred
  MB, reused by your other Flatpaks). It's newer and less-tested for this game, so if anything misbehaves,
  use the AppImage. Grab `KeeperFX-Tux-Edition-x86_64.flatpak` from
  [Releases](https://github.com/ForkedInTime/keeperfx-tux-edition/releases) and:
  `flatpak install --user ./KeeperFX-Tux-Edition-x86_64.flatpak`
  *(The Flatpak tracks the **stable** channel and is rebuilt monthly, so it may briefly trail a
  brand-new stable — no matter: it self-updates the game package on launch.)*

**Short version: use the AppImage unless you specifically want Flatpak.**

## The complete package (manual install)

The engine and game data without the AppImage around them; you bring the libraries and your *Dungeon
Keeper* files.

1. Download **`keeperfx-linux-alpha-x86_64-full.7z`** from the [latest release](https://github.com/ForkedInTime/keeperfx-tux-edition/releases/latest).
   (It keeps the project's first name because every installed launcher downloads it by that name.)
2. Extract it into `~/.local/share/keeperfx-alpha` — its files sit at the top of the archive:
   ```bash
   mkdir -p ~/.local/share/keeperfx-alpha
   7z x keeperfx-linux-alpha-x86_64-full.7z -o"$HOME/.local/share/keeperfx-alpha"
   ```
3. Add your **own** Dungeon Keeper files into `data/` (`bluepal.dat`, `slab0-0.dat`, the palettes…) and
   `sound/` (`atmos1.sbk`, `atmos2.sbk`, `bullfrog.sbk`).
4. Install the runtime libraries (Arch shown; names vary by distro):
   ```bash
   sudo pacman -S --needed sdl3 sdl3_mixer sdl3_image \
     ffmpeg openal luajit libspng minizip zlib miniupnpc libnatpmp openssl zstd
   ```
5. Run: `cd ~/.local/share/keeperfx-alpha && ./keeperfx`

## System requirements

- **A current 64-bit Linux distro: Ubuntu 24.04 / 26.04 or newer, Fedora, Arch, Steam Deck**, etc.
- ⚠️ **Will NOT run on Ubuntu 22.04 or older.** Official builds are compiled on Ubuntu 24.04, whose C
  library (glibc 2.39) is newer than what 22.04 and earlier provide — the binaries simply won't start on
  them. Older systems must [build from source](development.md#build-from-source) instead.
- An OpenGL 3.3-capable GPU for the default renderer (`RENDERER=OPENGL`): any graphics card or integrated
  GPU from the last decade. Where OpenGL cannot start (an old GPU, a VM, a broken driver), the game switches
  to the software renderer on its own, says so in `keeperfx.log`, and runs; `RENDERER=SOFTWARE` skips the
  attempt.
- The system's own graphics (OpenGL, X11), sound (ALSA) and font (fontconfig, fribidi) libraries. Every
  desktop install has them; the AppImage leaves these to the host on purpose, so a bare container or a
  server image will not do.
