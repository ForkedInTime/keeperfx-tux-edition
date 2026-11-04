# KeeperFX — Tux Edition

*The native Linux build of KeeperFX — built for penguins. 🐧 &nbsp;Unofficial, community-maintained, and continuously re-synced with the upstream team's work.*

![KeeperFX Tux Edition](docs/assets/readme-banner-tux-edition.png)

[![Stable](https://img.shields.io/github/v/release/ForkedInTime/keeperfx-tux-edition?label=stable&style=flat-square&color=2ea44f)](https://github.com/ForkedInTime/keeperfx-tux-edition/releases/latest)
[![Alpha](https://img.shields.io/github/v/release/ForkedInTime/keeperfx-tux-edition?include_prereleases&filter=*-alpha&label=alpha&style=flat-square&color=orange)](https://github.com/ForkedInTime/keeperfx-tux-edition/releases)
[![Upstream](https://img.shields.io/badge/upstream-dkfans%2Fkeeperfx-blue?style=flat-square)](https://github.com/dkfans/keeperfx)
![Platform](https://img.shields.io/badge/platform-Linux%20x86__64-1793D1?style=flat-square)
![License](https://img.shields.io/badge/license-GPL--2.0-blue?style=flat-square)
[![This page last updated](https://img.shields.io/github/last-commit/ForkedInTime/keeperfx-tux-edition/alpha?path=README.md&style=flat-square&label=this%20page%20last%20updated&color=2ea44f)](https://github.com/ForkedInTime/keeperfx-tux-edition/commits/alpha/README.md)

**Dungeon Keeper, native on Linux.** No Wine. One file on most distros, a proper package on Arch.

## ▶ Download &amp; play

### **[⬇ KeeperFX-Tux-Edition-x86_64.AppImage](https://github.com/ForkedInTime/keeperfx-tux-edition/releases/latest/download/KeeperFX-Tux-Edition-x86_64.AppImage)** &nbsp;·&nbsp; ~500 MB &nbsp;·&nbsp; any current 64-bit Linux

```bash
chmod +x KeeperFX-Tux-Edition-x86_64.AppImage
./KeeperFX-Tux-Edition-x86_64.AppImage
```

That **one file** has everything — launcher, engine, every library, and the game data. Nothing to install.
It opens the launcher, which finds your *Dungeon Keeper* files, lets you set up graphics and sound, and plays.

**On Arch, CachyOS, EndeavourOS or Manjaro**, install the package instead — the launcher and game data come with it,
and updates arrive with your normal `yay -Syu`:

```bash
yay -S keeperfx-tux
```

Then open **KeeperFX** from your menu. **Prefer Flatpak?** Grab `KeeperFX-Tux-Edition-x86_64.flatpak` from
[the releases](https://github.com/ForkedInTime/keeperfx-tux-edition/releases).
Details for each, and a manual install: **[Installing »](docs/install.md)**

<details>
<summary><b>Won't start?</b> ("Cannot mount AppImage, please check your FUSE setup")</summary>

> The AppImage needs FUSE in the kernel (`/dev/fuse`), which every desktop install has — but not the old
> `libfuse2` package many guides tell you to install. Somewhere without FUSE (a container, a locked-down
> kernel), run it without mounting: `./KeeperFX-Tux-Edition-x86_64.AppImage --appimage-extract-and-run`
</details>

### 🔑 You need your own *Dungeon Keeper*

KeeperFX runs on the original game's files, which nobody may give away: buy it on
[GOG](https://www.gog.com/game/dungeon_keeper),
[Steam](https://store.steampowered.com/app/1996630/Dungeon_Keeper_Gold/) or
[EA](https://www.ea.com/games/dungeon-keeper/dungeon-keeper), or use an old CD, and the launcher copies the
files for you. There is no GOG Galaxy for Linux — see
**[How do I install DK1 on Linux?](docs/faq.md#how-do-i-install-dk1-on-linux)**

Runs on Ubuntu 24.04 or newer, Debian 13, Fedora, Arch, Steam Deck… — not Ubuntu 22.04 or older
([requirements](docs/install.md#system-requirements)).

## 🧭 Stable or alpha?

The download above is **stable**: it only moves once a build has been played and left alone for a while.
**Alpha** is where the work happens and can change under you; its releases are tagged `-alpha`. The
launcher's Settings dialog has a release-channel setting, and it only ever offers you releases from the
channel you picked. The AUR package and the Flatpak follow stable. Not sure? Stay on stable.

**Stable is 1.4.0.6072 (3 November 2026).** The badges above track both channels live.

## ❓ Help

- **Will my saved games carry over?** Campaign progress does. Saves made before the October 2026 save
  format open only in the version that wrote them, and that version is kept when you update — see
  [the FAQ](docs/faq.md#can-i-switch-between-stable-and-alpha-without-losing-my-saved-games).
- **Something broke?** The game's log is `keeperfx.log` in its folder, and the last five runs are kept
  ([where to find it](docs/faq.md#where-is-the-games-log-and-how-do-i-report-a-bug)).
  [Open an issue](https://github.com/ForkedInTime/keeperfx-tux-edition/issues) here with it attached —
  please don't report this edition's problems to the KeeperFX team.
- **More questions:** [FAQ](docs/faq.md).

## What's different from upstream KeeperFX

The *game* is the KeeperFX team's. Upstream supports Linux as source code but releases only a Windows
build; this edition is the Linux half they don't ship, plus the bugs found on the way:

- 🐧 **Ready-to-run Linux builds** — the AppImage, a Flatpak and an Arch package, with a native Qt launcher
  that doesn't touch Wine.
- 🔄 **Kept current** — every week a bot merges the team's latest `master`, with its Windows-only parts
  stripped out and proven not to change the game.
- 🛠️ **Linux fixes** — ~20 places where code written for 32-bit Windows broke on 64-bit Linux, ~60
  correctness and security fixes (crafted maps and mods, map scripts that could change your files or run code
  outside the game, chat and replays that could act on your machine), ~30 gameplay bugs found in players'
  games, and ~10 crashes.
- ⚡ **Performance** — the graphics card draws the game by default, frame pacing matched to your monitor, and
  the map view no longer costs a processor core.
- 🧰 **Launcher** — Workshop browser, Mod Manager, and a built-in updater with separate stable and alpha
  channels.

454 commits on top of the team's `master`. **[Everything, item by item »](docs/whats-different.md)**

## Credit

**KeeperFX is the work of the KeeperFX team and the Keeper Klan community**, built up over many years: the
complete decompilation-and-rewrite of *Dungeon Keeper* into modern code, the gameplay, the tooling — **and the
native Linux support this edition stands on.** The overwhelming majority of everything you run here is
theirs. If you enjoy this, support **the upstream project** — that's where the game is actually made:
[github.com/dkfans/keeperfx](https://github.com/dkfans/keeperfx) ·
[keeperfx.net](https://keeperfx.net) ·
[Keeper Klan Discord](https://discord.gg/hE4p7vy2Hb)

> ⚠️ *Unofficial personal fork — **not** affiliated with the KeeperFX team.*

## 🤖 On AI-assisted development

**Yes, AI assistance is used here, heavily.** What people rightly call "AI slop" is *unverified output
pushed onto other people* — and none of that happens here: **no patches from this fork are pushed at the
KeeperFX team** (the weekly sync runs inbound only), and every change is compiled and **played on real
Linux hardware** before it ships. **Judge the diff, not the toolchain** — it's all right there, one commit
at a time. [The longer answer, with sources »](docs/ai-assisted-development.md)

## More

- [Installing](docs/install.md) — AppImage, AUR, Flatpak, a manual install, system requirements
- [What's different from upstream](docs/whats-different.md) — every change, area by area
- [FAQ](docs/faq.md) — saves across versions, installing *Dungeon Keeper* on Linux, logs
- [Building and maintaining](docs/development.md) — build from source, `linux.mk`, how the weekly sync works
- [Changelog](CHANGELOG.md) — every release
- The launcher has its own repository: [keeperfx-launcher-qt-linux](https://github.com/ForkedInTime/keeperfx-launcher-qt-linux)

## License

**GPL-2.0-or-later**, same as upstream KeeperFX — see [LICENSE](LICENSE). All upstream copyrights remain with
the KeeperFX team and contributors. The banner is adapted from the official KeeperFX banner (re-labelled for
the Tux Edition) with thanks. *Dungeon Keeper* is a trademark of its respective owners; this project is not
affiliated with them.
