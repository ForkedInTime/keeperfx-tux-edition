# FAQ

<sub>[← Back to the README](../README.md)</sub>

## Can I switch between stable and alpha without losing my saved games?

Yes, and you can switch back. Switching never modifies or deletes a save file — but a save made
before the October 2026 save format **opens only in the version that wrote it**, and a build from
before that change cannot read the newer saves at all. Since stable 1.4.0.6062 (3 November 2026)
both channels use the new format; saves from stable 1.4.0.5660 and earlier are on the other side of
it. One newer exception: since stable 1.4.0.6072 a level's script functions are saved as their text,
so a save on a level whose script keeps functions (Pandaemonium and Heroes' Journey levels among them)
opens in 1.4.0.6072 and later, but not in 1.4.0.6062 or the 1.4.0.6013 alpha. Saves on other levels
are not affected.

In practice: a campaign tells you it needs the latest alpha, you switch to alpha, play it, then
switch back to stable — and your stable saves are exactly where you left them. What you cannot do,
across that change, is start a campaign on one channel and continue it on the other. Campaign
progress — the *Continue* button — is stored apart from saved games and carries over when you update.

This is not a policy, it is the save format: an older save is a raw copy of the engine's internal
state, so it only makes sense to an engine laid out the same way. Saves in the new format describe
their own layout, so they keep loading in later versions. When a save cannot be loaded the game
tells you so, and the engine that *can* read it is kept when you update — so the campaign is a
version switch away, not lost.

## How do I play a campaign my current version can no longer load?

Run the version that saved it. Updating keeps the previous engine automatically, three releases
deep, so it is already on your machine:

- **Arch (AUR)** — `keeperfx-tux-previous` lists what is kept and sets up an isolated copy of the
  game to play one. Its saves are separate from your current game and it cannot write to it.
- **AppImage / Flatpak** — the last three engines are kept inside the game folder, in `previous/`
  ([where the game folder is](#where-is-the-games-log-and-how-do-i-report-a-bug)). There is no
  command for them yet: play one from a copy of the game folder, so it cannot write to the saves of
  the game you play now.

`keeperfx-tux-previous` copies nothing out of your current install without being asked, and nothing
you do in its copy can touch the campaigns you are playing now.

## How do I install DK1 on Linux?

KeeperFX needs your **own original *Dungeon Keeper* files** as proof of ownership — the launcher copies
them out of a real install. On Windows you'd just run GOG Galaxy; **there is no GOG Galaxy for Linux**, so
you install your GOG/Steam/EA copy with a Wine front-end first, then point the launcher at it.

**Option A — Lutris (recommended for GOG)**
1. Install Lutris: `sudo apt install lutris` (Ubuntu/Debian), `sudo pacman -S lutris` (Arch), or the Flatpak.
2. Link your GOG account: **Preferences → Sources → GOG → Sign in**. This lets Lutris pull the installer
   automatically.
3. Open [Dungeon Keeper on lutris.net](https://lutris.net/games/dungeon-keeper/), pick a **GOG** installer
   (*GOG (Auto)* if your account is linked, otherwise the *DOSBox GOG* one), click **Install** and follow
   the script. Any of them puts the original files on disk; the one called *Wine GOG + KeeperFX* also
   installs the Windows KeeperFX, which you don't need for this edition.
4. **If Lutris can't auto-download** (account not linked, offline installer, GOG API hiccup), it shows a
   file picker. Point it at the GOG **setup `.exe`** you downloaded from gog.com (plus any `.bin` parts) —
   *not* an existing install folder. Lutris installs the game fresh into its own Wine prefix.

**Option B — Heroic Games Launcher**
1. Install Heroic (`flatpak install flathub com.heroicgameslauncher.hgl`, or the AppImage).
2. Log in to **GOG** in Heroic's sidebar.
3. Select *Dungeon Keeper* from your GOG library and **Install** — Heroic downloads it and sets up the
   Wine prefix for you.

**Option C — Extract the GOG offline installer (no Wine at all)**
If you have the GOG **offline installer** `.exe`, you don't even need to run it — unpack it directly:
```bash
sudo apt install innoextract        # Ubuntu/Debian (or your distro's package)
innoextract setup_dungeon_keeper_gold_*.exe
```
It unpacks into the current folder (some installers put the game in an `app/` subfolder); point the
launcher at the folder that holds `data/` and `sound/`. An installer that came with `.bin` parts needs
`innoextract --gog`. This is the lightest option — no GOG client, no Wine.

**Option D — Steam + Proton**
Own it on [Steam](https://store.steampowered.com/app/1996630/Dungeon_Keeper_Gold/)? Enable **Proton** for it
(Properties → Compatibility → *Force the use of a specific Steam Play compatibility tool*) and install — the
launcher detects the Steam copy.

**Then connect it to KeeperFX**

The KeeperFX launcher tries to **auto-detect** your install and copy the game files. If it can't find them,
it asks you to point it at the folder yourself — so locate the install inside the Wine prefix, typically:

- **Lutris:** `~/Games/dungeon-keeper/drive_c/GOG Games/Dungeon Keeper/` (or wherever you installed it)
- **Heroic:** the game's own folder under `~/Games/Heroic/` (Heroic keeps the Wine prefix apart from it,
  under `~/Games/Heroic/Prefixes/`)

That folder (and its `data/` and `sound/` subfolders) is what the launcher copies from. You only do this
once; after that KeeperFX runs natively with **no Wine**.

> 💡 You don't keep using Lutris/Heroic to *play* — they're only there to install the original game so
> KeeperFX can borrow its data files. Once copied, launch KeeperFX directly.

## Where is the game's log, and how do I report a bug?

The game writes `keeperfx.log` in its folder: `~/.local/share/keeperfx-alpha` for the AppImage and the
AUR package, and `~/.var/app/io.github.forkedintime.KeeperFXLinuxAlpha/data/keeperfx-alpha` for the
Flatpak. It keeps the previous five runs as `keeperfx.log.1` to `keeperfx.log.5`, so the log of a
session that went wrong survives starting the game again. The launcher shows it too: **Play ▾ → View logs**, beside its own.

Report bugs in this fork **[here](https://github.com/ForkedInTime/keeperfx-tux-edition/issues)**, with
the log attached, and not to the KeeperFX team: they don't build or ship this edition.

## Can I play online with friends on Windows?

Not with this edition. Players of KeeperFX: Tux Edition play each other, and Windows players play each
other on the official KeeperFX.

Having the same maps isn't enough. In a multiplayer game the computers only send each other what each
player did, and every computer works out the rest itself: where each creature walks, who wins each
fight. That only works if every player runs exactly the same game code. This edition fixes bugs in that
code, so the two would drift apart. The game refuses to join a different build for that reason, and the
two editions also find games through different lobbies.

To play with Windows friends, install the official Windows KeeperFX from
[keeperfx.net](https://keeperfx.net) and run it under Wine, Lutris or Bottles. That is their software and
their online service, separate from this edition.

## Why is my game folder called `keeperfx-alpha`?

That was this project's first name, before it had a stable channel (the Flatpak's ID keeps it too).
The folder holds every install's saves, settings and add-ons, so it keeps the name rather than move
them; stable and alpha builds both use it. The AppImage can use another folder:
[`KFX_ALPHA_PREFIX`](install.md#keep-the-game-in-another-folder-lutris-another-drive).
