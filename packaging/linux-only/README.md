# Linux-only: how Windows content is kept out of this fork

KeeperFX is developed for Windows first. The team's repository carries a Visual
Studio project, MinGW and MSVC build setups, Windows CI, a Windows debugger
binary, and `#ifdef _WIN32` branches throughout the engine. None of it is used
by this fork, which builds one thing: a native Linux binary, with `linux.mk`.

Everything here exists so that none of that content is in this repository, and
so that the weekly sync with the team cannot bring it back.

## The short version

- **The sync never merges the team's commits directly.** Each week it takes their
  tree, removes the Windows content from it, commits the result as a
  *Linux-only snapshot*, and merges that.
- **What gets removed** is listed in [`remove.list`](remove.list) (files and
  directories, each with its reason) and [`windows-macros.txt`](windows-macros.txt)
  (the preprocessor symbols whose `#if` branches are deleted).
- **It is proven not to change the game.** [`prove-equivalence.sh`](prove-equivalence.sh)
  compares what the Linux compiler sees with and without the filter, file by file,
  and requires it to be identical. The sync runs it every week.
- **It cannot come back by accident.** [`check-tree.sh`](check-tree.sh) fails if a
  Windows file or a Windows `#if` is present; the *Linux-only guard* workflow runs
  it on every pull request into `alpha` and every push to it.

## Why snapshots, and not "merge, then delete"

The obvious approach -- merge the team's `master`, then delete their Windows
files -- was how this fork started, and it failed in two ways:

1. **Every week re-fights the same conflict.** Delete a file the team still edits
   (their Visual Studio project changes several times a month) and each of their
   edits is a modify/delete conflict. Deleting their five Windows CI workflows by
   hand caused exactly that: they were 3 of the 5 conflicting files in the sync
   issue of 2026-09-21.
2. **New Windows files arrive silently.** A file the team *adds* merges without
   any conflict. A new CI workflow would even run here, with this repository's
   secrets.

A snapshot avoids both. Its parent is the previous snapshot, so when the fork
merges it the merge base is the previous *filtered* tree: Windows files and
`#ifdef` blocks are on neither side of the merge, so they can neither conflict
nor be re-imported. The fork only ever sees the Linux-relevant part of what the
team changed.

```
team (dkfans/keeperfx)  ──A──B──C──D──E──F──▶            their history
                              │           │
                       filter │    filter │
                              ▼           ▼
snapshots               ──── S1 ───────── S2 ──▶          one commit per sync
                              ╲             ╲
fork (alpha)            ──●────●──●──●────────●──▶      merges S1, then S2
```

Each snapshot records where it came from in two trailers:

```
Linux-Snapshot-Of: <full sha of the team's commit>
Linux-Snapshot-Upstream-Count: <git rev-list --count of that commit>
```

The first one ("bootstrap", of `d19666f9a`, the commit the fork had last merged
directly) was merged on 2026-10-10 and removed the Windows content the fork had
accumulated until then.

## What the filter does

[`filter-tree.sh`](filter-tree.sh) works on a git work tree in two passes.

**Paths.** Everything in `remove.list` is deleted. The `[upstream-only]` section
(the team's `.github/`) applies only to their snapshots -- the fork keeps its own
`.github/` -- while the `[windows]` section applies everywhere. If a deleted
engine header is `#include`d outside any `#ifdef` (the engine includes
`PlatformWindows.h` for a class it only uses on Windows), that `#include` line is
dropped too.

**Code.** In every C/C++ file that mentions a symbol from `windows-macros.txt`,
[unifdef](https://dotat.at/prog/unifdef/) resolves each `#if`/`#ifdef`/`#elif`
those symbols decide as "not defined" -- which is what GCC and Clang on Linux
already see -- and deletes the branch that can never be compiled here. unifdef
leaves a condition alone when it also tests something it does not know, such as
`#if defined(__LP64__) || defined(_WIN64)`. [`simplify_conditions.py`](simplify_conditions.py)
handles those: it substitutes `0` for the Windows symbols (what the C standard
says an undefined identifier is in `#if`), folds only what that makes exact, and
writes back the shorter condition (`#if defined(__LP64__)`) or a marker unifdef
then resolves. A condition it cannot parse is left alone and reported.

What is deliberately **kept**:

- Upstream's `Makefile` and its `pkg_*.mk`/`tool_*.mk` includes. Their default
  target is the MinGW build, but `make -f Makefile pkg-gfx` is what regenerates
  the game's artwork from FXGraphics in the release workflows, and nothing else
  drives that pipeline yet. Replacing it with a Linux-only equivalent is possible
  future work; until then these files stay, and `GNUmakefile` makes a bare `make`
  build Linux.
- `res/*.png` (the AUR package installs these icons) and everything under
  `tools/fxfontmaker/` except its `.bat` file.
- Non-C code with platform checks (shell, Python, Lua): it has no preprocessor
  and its checks are not ours to rewrite.

## The proof

[`prove-equivalence.sh`](prove-equivalence.sh) `--upstream|--fork <commit>`
checks the commit out twice and runs one copy through the filter with
`--keep-lines` (removed lines become blank lines, so line numbers and `__LINE__`
do not move). Then every file the Linux build compiles -- `linux.mk`'s sources,
the TOML library and the GL loader, plus any new source in the team's tree -- is
run through the preprocessor in both copies with `linux.mk`'s own flags:

- identical preprocessor output means identical compiler input;
- where the text differs only by declarations (the dropped `#include` of an
  unused class), the object code must be byte-identical instead;
- a file the filter deletes must not be one `linux.mk` compiles.

Results when this was introduced: on the team's `d19666f9a`, on this fork, and on
the team's `fb0ab7f4d` (48 commits later), every compiled file passes -- 311, 311
and 315 files preprocessing identically, `PlatformManager.cpp` compiling to
identical object code. Marking `__linux__` as a "Windows" symbol, as a test, makes
it fail.

## The weekly sync

[`.github/workflows/upstream-sync.yml`](../../.github/workflows/upstream-sync.yml),
Mondays 06:00 UTC:

1. Compares the team's `master` with the commit the last snapshot came from
   ([`synced-upstream.sh`](synced-upstream.sh)). Nothing new: done.
2. [`make-snapshot.sh`](make-snapshot.sh) builds the snapshot. If the team changed
   only Windows-only things, the filtered tree is unchanged and the sync stops
   quietly.
3. Merges it into a throwaway branch off `alpha`, and runs
   [`sync-linux-mk-sources.sh`](sync-linux-mk-sources.sh): sources the team added
   are appended to `linux.mk`'s list and deleted ones dropped. That used to be a
   manual step after every sync that added a file.
4. Checks: `check-tree.sh`, a full build, `tests/run.sh`, `prove-equivalence.sh`,
   and the save-format check.
5. All green: a pull request. Anything else: an issue -- or, if a sync issue is
   already open, a comment on it, so a stuck sync does not open one issue a week.

**Merge sync pull requests with a merge commit, never squash them.** The next
sync builds on the snapshot commit inside the pull request; a squash leaves it
out of `alpha`'s history.

## Syncing by hand

When the sync reports conflicts, resolve them like this (needs `unifdef`:
`sudo pacman -S unifdef` or `sudo apt install unifdef`):

```bash
git fetch origin && git switch -c sync/upstream-$(date +%F) origin/alpha
git remote add upstream https://github.com/dkfans/keeperfx.git 2>/dev/null
git fetch upstream master
git merge "$(packaging/linux-only/make-snapshot.sh upstream/master)"
# resolve conflicts, then:
packaging/linux-only/sync-linux-mk-sources.sh
packaging/linux-only/check-tree.sh
git commit
```

**Never `git merge upstream/master` directly** -- that brings their history, and
with it the Windows content, back. (`check-tree.sh` and the guard workflow catch it
if it happens.)

## Build numbers

The fourth part of the version (`1.4.0.NNNN`) used to be
`git rev-list --count HEAD`, which counted the team's commits because their history
was merged in. Snapshots do not bring their commits in, so
[`build-number.sh`](build-number.sh) adds them back from the snapshot trailers:

```
rev-list --count HEAD + (team's count at the newest snapshot − team's count at the first)
```

It equals the old number until the first sync after the bootstrap and only ever
grows. keeperfx-launcher-qt enables settings by build number, measured against the
team's numbering, so this matters. Every build -- CI, the AUR PKGBUILD,
`refresh-alpha.sh` -- uses it (falling back to the plain count for tags made before
it existed).

## Changing what is removed

- **Remove something more:** add it to `remove.list` with a comment saying why it is
  safe, then run `selftest.sh`, `filter-tree.sh --fork .`, `check-tree.sh` and
  `prove-equivalence.sh --fork HEAD`. Check that nothing in the Linux build, the
  release workflows or `packaging/` refers to it -- the filter cannot know.
- **Bring something back:** delete its line. The next snapshot contains the path,
  and merging it adds it to the fork like any other change from the team.
- **A new Windows-only macro:** add it to `windows-macros.txt` only if no Linux
  compiler predefines it and nothing in the tree `#define`s it; the proof will tell
  you if that is wrong.

## Files

| File | Purpose |
|---|---|
| `remove.list` | paths removed, by section, with reasons |
| `windows-macros.txt` | preprocessor symbols treated as undefined |
| `filter-tree.sh` | the filter (`--upstream` for snapshots, `--fork` for this tree) |
| `simplify_conditions.py` | reduces mixed `#if` conditions unifdef leaves alone |
| `check-tree.sh` | fails on any Windows content |
| `prove-equivalence.sh` | proves the filter does not change the Linux build |
| `make-snapshot.sh` | builds a snapshot commit of the team's tree |
| `synced-upstream.sh` | the team's commit this checkout last synced to |
| `build-number.sh` | the build number, in the team's numbering |
| `sync-linux-mk-sources.sh` | keeps `linux.mk`'s source list in step with the tree |
| `early-port-followups.py` | fails a release that took a team commit early but lacks the team's later fix to it (`docs/releasing.md`) |
| `early-port-followups.list` | the verdict for each such pair: `port` or `skip`, with the reason |
| `selftest.sh` | self-test on a synthetic repository |
| `lib.sh` | shared helpers |
