# Cutting a release

Two channels share one line of history. Which one a build lands in is decided
entirely by **the tag and the prerelease flag** — nothing else in the pipeline
infers it.

| Channel | Tag | GitHub release | Reaches |
|---|---|---|---|
| stable | `v1.4.0.5423` | **not** prerelease | AppImage · full.7z · AUR · Flatpak |
| alpha  | `v1.4.0.5397-alpha` | **prerelease** | AppImage · full.7z |

## Steps

0. **If the launcher changed, push its `alpha` branch first.** See below — this is
   the step that is easy to forget and produces a release that looks fine.
1. **Check the team's fixes to anything taken early.** On the commit you are about to
   tag, with the team's history fetched (`git fetch upstream`):
   `packaging/linux-only/early-port-followups.py <commit>`. It must say `ok`. A
   `MISSING` line is a team fix the release has to carry first; an `UNREVIEWED` line
   needs a verdict in `packaging/linux-only/early-port-followups.list` (see below).
2. **Bring the pages up to date**, in both repositories, against every commit since each
   page last changed: see [Keeping the pages current](#keeping-the-pages-current). The
   commit count goes in last.
3. Commit the changelog entry and any version change.
4. Tag it. A stable tag carries no suffix; an alpha tag ends in `-alpha`.
5. Create the GitHub release for that tag — **and tick prerelease for an alpha.**
6. CI does the rest: `build-appimage.yml` builds and attaches the AppImage and
   `full.7z`; `publish-aur.yml` updates the AUR recipe. The Flatpak is monthly
   and self-updates, so it is not part of cutting a release.

## The launcher ships from its own branch

The launcher is a **separate repository**
(`ForkedInTime/keeperfx-launcher-qt-linux`), and `build-appimage.yml` clones it at
a branch:

```yaml
LAUNCHER_BRANCH: ${{ inputs.launcher_branch || 'alpha' }}
git clone --branch "$LAUNCHER_BRANCH" --depth 1 "$LAUNCHER_REPO" launcher
```

A release always builds `alpha`. To try a launcher branch in a real AppImage
before it is merged, run the workflow by hand with that branch; it attaches
nothing to a release, and its artifacts carry the branch in their names
(`KeeperFX-Tux-Edition-AppImage-launcher-<branch>`), so they cannot be taken for a
release's:

```bash
gh workflow run build-appimage.yml --repo ForkedInTime/keeperfx-tux-edition \
  --ref alpha -f launcher_branch=<branch>
```

The launcher repository used to keep its own copy of this workflow for that; it
had drifted far from this one and is gone.

Day-to-day launcher work happens on its **`master`**. So a release only carries
launcher changes if `alpha` has been fast-forwarded to `master` first:

```bash
git -C <launcher> push origin master:alpha
```

**Why this deserves its own step:** nothing fails if you skip it. The engine
release builds, every workflow goes green, the assets attach, and the changelog
describes launcher features that are not in the binary. It looks like the
features are broken rather than absent. This happened while cutting
`v1.4.0.5465-alpha`: `alpha` was six commits behind and the whole launcher
release would have shipped without a single one of its changes.

Two ways to confirm it worked, in order of effort:

- before tagging: `git -C <launcher> rev-list --count origin/alpha..origin/master`
  should be `0`;
- after the build: the AppImage run log should show the new sources compiling,
  e.g. `Building CXX object ... logviewerdialog.cpp.o` for the release that added
  the log viewer.

## Keeping the pages current

Players read the README and the docs pages, not the changelog, so a page that lags is
wrong information for everyone who reads it. They are brought up to date at **every**
cut, alpha or stable, in both repositories:

| Repository | Pages |
|---|---|
| this one | `README.md`, `docs/install.md`, `docs/whats-different.md`, `docs/faq.md`, `docs/development.md` |
| launcher | `README.md`, `docs/changes.md`, `docs/screenshots.md`, `docs/development.md` |

1. **Read what changed, then every page in full.** For each page,
   `git log --no-merges <the page's last commit>..HEAD` is what it has not seen yet.
   Every change a player can notice gets its line item in `docs/whats-different.md`
   *and* its row in the table at the top of that page. Check each row against its
   section rather than leaving it as it was: the Multiplayer row still read "map packs,
   1 fix" a week after the lobby server shipped. Team fixes taken ahead of a sync count
   too. Then read each page from top to bottom and check every claim against the code,
   the packaging or the release itself, including the instructions for other programs
   (Lutris, Heroic, innoextract) against their own pages. A page with no new commits
   can still be wrong: in November 2026 the install page told Arch users to run a
   release engine that cannot start there, and called a Flatpak bundle self-updating.
2. **Facts that change without a commit here.** At a stable cut, the README's
   "Stable is …" sentence. The Flatpak is built on the 1st of each month and goes onto
   whichever release is the latest at that moment, so the README sends Flatpak users to
   the newest release *that has one*. Every claim about upstream (what their releases
   contain, their AppImage pull request) is checked against upstream itself.
3. **Pictures.** The launcher's pictures (two in its README, seventeen more on its
   screenshots page) are all retaken whenever anything visible in the launcher changed
   since the version they show; `git log <the pictures' commit>..origin/alpha` in the
   launcher repository answers that. They are taken on the released build, from a
   throwaway copy of a game folder, and each set gets new file names carrying the
   version (GitHub's image cache keeps serving an overwritten file), with the old files
   removed. A fix of this edition's that shows on screen gets a before/after picture in
   `docs/whats-different.md` **when it is made**: the "before" half needs the old build,
   and an old release build stops starting on a rolling distribution once its libraries
   move on (by November 2026 the 1.4.0.6072 and 1.4.0.6078 engines, built on
   Ubuntu 24.04, missed six libraries on Arch). Pictures carry no metadata and show
   only this edition's own work.
4. **The count, last.** `git log --oneline --no-merges --invert-grep
   --grep='^Linux-Snapshot-Of:' upstream/master..HEAD` here (it is in `README.md` and
   `docs/whats-different.md`), and `git log --oneline --no-merges upstream/master..HEAD`
   in the launcher repository (its `README.md` and `docs/changes.md`), each counting the
   commit that writes it. Change the number by its text, not its line number: an edit
   higher up the page moves the line. Bot commits that CI adds after a cut (the AUR
   recipe, the library watch) leave it low until the next page commit.
5. **Check, then look at the live page.** Every relative link and anchor must resolve.
   After the merge, the page on GitHub must show the change. The launcher repository's
   default branch is `alpha`, so its front page is `alpha`'s README, not `master`'s.

## Why the prerelease flag matters

GitHub defines `releases/latest` as *the newest release that is not a
prerelease*. Several things resolve through it:

- the README's download button, which promises the newest **stable** build
- `build-appimage.yml` and `build-flatpak.yml`, which fetch the previous
  `full.7z` from `releases/latest` to use as the payload base

Alpha releases were not flagged, so `latest` simply meant *most recently
published*. Because alphas ship far more often than stables, the next alpha
would have taken over that link — handing stable users an alpha, and building
the next stable's payload on top of an alpha's. All 27 existing alpha releases
were flagged retroactively on 2026-08-08; keep new ones flagged.

## The artwork is regenerated from FXGraphics on every build

Upstream keeps the game's artwork in a separate repository, `dkfans/FXGraphics`,
and its own alpha workflow regenerates every sprite sheet, texture and land view
from it on each run (`make pkg-enginegfx` and friends in `pkg_gfx.mk`). Their
stable release package carries the sheets as they were at release time.

Our payload used to take its `data/` from upstream's *latest stable release*,
while the engine in the same package is built from upstream's *master*. Any
upstream change that pairs code with new art therefore shipped here with the
code and without the art. Found 2026-09-26: upstream #5106 (13 August) added an
in-game Load/Save scrollbar drawn from six new `gui1` sprites; the July release
sheet has 239 entries, the code wanted 245, every log carried "Reserved button
sprite 239 has no matching entry in the loaded spritesheet", and a player with
more than eight saves got no scrollbar at all. Eight FXGraphics commits since
July were missing by then.

Both release workflows now run `make -f Makefile pkg-gfx` after building the
engine (it shallow-clones FXGraphics, about 930 MB, and fetches the prebuilt
Linux converters) and lay `pkg/data`, `pkg/ldata` and `pkg/campgns` over the
payload *after* the upstream release overlay. `-f Makefile` matters: a bare
`make` is forwarded to `linux.mk` by the GNUmakefile. The AUR data package is
cut from the release archive, so it inherits the regenerated artwork.

## Why the AUR only gets stable

`publish-aur.yml` skips any tag ending in `-alpha` or `-prototype`.

pacman upgrades on version alone. Before this filter existed, an alpha tag was
published to the AUR with its suffix stripped — `v1.4.0.5397-alpha` became
pkgver `1.4.0.5397` — so `yay -Syu` moved Arch users onto alpha builds silently,
indistinguishably from stable, at alpha cadence. The launcher has a
release-channel setting so a user can opt into alphas; a distro package has no
equivalent, so the filtering happens at publish time.

If Arch users should ever be able to follow the alpha line, that wants a
separate `keeperfx-tux-alpha` package they opt into by name — the usual Arch
convention — not a silent version bump.

## The stable-release trap: `latest` becomes the new release immediately

`build-appimage.yml` builds each release's payload by overlaying onto the previous
release's `full.7z`. It used to fetch that base from `releases/latest` — which is
safe for an alpha, because a prerelease never becomes `latest`, and wrong for a
stable, because publishing a stable makes it `latest` *the instant it is
published*, before its own assets exist. The build then tried to download its own
payload, got a 404, and `wget` exited 8 with no useful message.

This bit the first stable patch, `v1.4.0.5425`. The workflow now asks the API for
the newest **non-prerelease** release that actually carries
`keeperfx-linux-alpha-x86_64-full.7z`, skipping the tag being built, so it can
never resolve to itself — and still never bases a stable payload on an alpha's,
which is the rule the prerelease flag exists to enforce.

If you ever need the old manual escape: flag the new stable as prerelease, re-run
the failed build so `latest` falls back to the previous stable, then clear the
flag.

## A newer launcher for the current stable, without a new engine

Every stable ships `alpha`'s launcher with a stable engine. When the launcher gains
fixes between stables, the AUR can carry them to stable users as a packaging-only
release — same engine tag, next `pkgrel` — without cutting a new stable:

1. Merge the launcher work into the launcher's `alpha` (and fast-forward `master`).
2. On the **stable's own branch** (for 1.4.0.5660, `hotfix/1.4.0.5660`), bump
   `pkgrel` in `packaging/aur/PKGBUILD` past the live one and set `_launcher_commit`
   to the launcher's new `alpha` head; update `.SRCINFO` to match.
3. **Give that branch `alpha`'s `publish-aur.yml`** if it does not have it already
   (`git checkout origin/alpha -- .github/workflows/publish-aur.yml`, commit). A
   hand-run publish runs the workflow *of the branch it is dispatched on*, and a
   stable's branch keeps the workflow it was cut with. Before 2026-10-11 that
   workflow rebased its bookkeeping commit onto `alpha` and carried on when the
   rebase stopped in conflict — then copied `packaging/aur` from the conflicted
   tree, conflict markers and all, to the AUR. It stayed harmless only while the
   committed recipe matched what the workflow regenerates exactly. `alpha`'s
   workflow stages the files before anything else, records the bump on the branch
   it was run from, aborts a rebase that conflicts, and waits until the AUR offers
   the new version. (`hotfix/1.4.0.5660` has it since 5660-3.)
4. Run **Publish to AUR** by hand on that branch (`workflow_dispatch`, ref
   `hotfix/1.4.0.5660`, tag `v1.4.0.5660`) — a dry run first:
   ```bash
   gh workflow run publish-aur.yml --repo ForkedInTime/keeperfx-tux-edition \
     --ref hotfix/1.4.0.5660 -f tag=v1.4.0.5660 -f dry_run=true
   ```
   then again with `dry_run=false`.

Not from `alpha`: a stable's recipe belongs with its engine. `alpha`'s recipe has
moved on (it no longer lists `libepoxy`, which 1.4.0.5660 links), and the workflow
checks out the branch it is dispatched on. The workflow keeps the recipe's `pkgrel`
only when the recipe already records the version being published, which the
stable's branch does.

## The AUR bookkeeping step used to gate the AUR push

`publish-aur.yml` commits the version/checksum bump back to this repo and *then*
pushes to the AUR. That commit pushed straight at the default branch, so any
commit landing there in between made it fail — and because the AUR push is gated
on that step, the package silently never went out. The push is now rebased and
non-fatal; a failure warns and the AUR push still runs.

It also always went to the default branch, so a stable cut from its own branch
had its bump rebased onto `alpha`, which conflicted, warned, and recorded it
nowhere: `alpha`'s recipe went on saying 1.4.0.5652 after 1.4.0.5660 shipped. The
bump now goes to the branch the release belongs to — the branch a hand-run
publish was started from, or for a release the branch whose head is the tagged
commit (the default branch when both match) — and only falls back to the default
branch, with a warning, when no single branch fits.

## A green publish means users are offered it

The AUR's git repository has the new recipe the moment the push is accepted,
but the package information AUR helpers read can lag behind it by several
minutes. 1.4.0.5660-2 was accepted at 00:12 UTC on 2026-10-11 and `yay -Syu`
still reported "nothing to do" minutes later, which looked like a failed publish.
The workflow's last step now polls the AUR's API (every 30 s, up to 15 minutes)
until all three packages report the published version, and fails with a message
saying so if they do not. Re-running the workflow re-checks without pushing
again: the bump step re-pins the launcher to its current `alpha` head, so a re-run
after the launcher moved finds a recipe that differs from the AUR's at the same
version; when the launcher pin is the only difference it publishes nothing and
just re-checks (any other difference under an unchanged version still fails). By
hand:

```bash
curl -s 'https://aur.archlinux.org/rpc/v5/info?arg[]=keeperfx-tux' | grep -o '"Version":"[^"]*"'
```

## Ordering trap

`publish-aur.yml` pins the release's `full.7z` by hash, but that asset is
attached by the AppImage build, which runs *after* the release is published. On
a fresh release the archive is legitimately not there yet, so the workflow logs
a notice and skips rather than failing. Re-run it once the AppImage build has
uploaded.

## The save-format guard

Until upstream #5424 (synced 2026-10-19) a saved game was a raw memory dump of
`struct Game`, loaded only if its length matched the running build, so a field
added to any struct it reached invalidated every save in existence. It happened
on 2026-08-19 and cost five campaigns in progress.

A save now describes its own layout and is read field by field: a field the
file lacks takes its default, one the build dropped is skipped. What can still
lose players' games is a format major-version bump, a renamed field without an
alias, a narrowed type, or a struct member missing from the field tables (it is
simply never saved). `packaging/ci/check-save-format.sh` compares what the
tables say with the committed copy in `packaging/ci/save-format/` and, on a
difference, prints upstream's own policy verdict on whether older saves still
load. `build-linux-alpha.yml` runs it on every release build and the weekly sync
runs it on its pull request. Run it yourself before tagging:

```bash
packaging/ci/check-save-format.sh
```

If it fails, read `packaging/ci/save-format-baseline`, then accept the change
with `--update` in the commit that makes it, saying what the policy check said.
A break is a user-visible decision: it belongs in the changelog, and the release
body gets an automatic "saves from earlier versions cannot be loaded" warning,
written by the release build (`--release-note <previous tag>`).

## Build system

This fork builds with `linux.mk` only; upstream's CMake build is removed by the
Linux-only filter (README, "Why this fork builds with `linux.mk` only").

Syncing upstream no longer means adding their new source files to `linux.mk`
by hand: the weekly sync runs `packaging/linux-only/sync-linux-mk-sources.sh`,
which appends sources the team added and drops ones they deleted, and the
Linux-only guard fails a pull request whose list is out of date. Flags are
still ours to carry: #5099 also needed `-Isrc`, which upstream's Makefile has
always had -- a change like that still shows up as a red compile check.

## Upstream sync

The sync merges Linux-only snapshots of the team's tree, never their commits
(`packaging/linux-only/README.md`). Two rules follow:

- merge sync pull requests with a **merge commit** -- the next sync builds on
  the snapshot commit inside it, which a squash would drop;
- to sync by hand, `git merge "$(packaging/linux-only/make-snapshot.sh
  upstream/master)"`, never `git merge upstream/master`.

The build number (`packaging/linux-only/build-number.sh`) keeps the team's
numbering across snapshots; use it rather than `git rev-list --count HEAD`.

### Team commits taken early, and the team's fixes to them

We take some of the team's fixes before the sync that would bring them
(`git cherry-pick -x`, so the commit names its origin). The team sometimes fixes
such a commit days later, and that fix arrives only with the next sync. A release
cut in between ships the early commit's bug without its cure: stable 1.4.0.6062 to
1.4.0.6098 carried the team's "creatures stop fighting when keepers ally" without
their fix for what it broke, and lair-enemy fights stopped after half a second for
every stable player from 3 to 6 November. The fix was in the 2026-11-05 sync, which
stable did not take; it shipped in 1.4.0.6103.

`packaging/linux-only/early-port-followups.py <rev>` lists, for a release, every
later team commit it lacks that touches the same functions (or the same data file)
as one it took early, names that commit's pull request, or reverts it. Each pair
needs a line in `early-port-followups.list`: `port` if the release must carry the
later commit, `skip` with the reason if not (a refactor or feature over the same
code). Read the later commit, its pull request and the release's own code before
writing a verdict. A `port` the release lacks fails the check, which is what stops
a repeat. `.github/workflows/early-port-check.yml` runs it every day on `alpha`,
the latest stable and any hotfix branch on it, and fails on 1.4.0.6098 on purpose
to prove it still catches the case it was written for.

So: when you take a team commit early, expect to answer for its follow-ups at every
cut until the sync that carries them, and treat a fix to an early commit as a fix
for the release, not as a note for the next sync.
