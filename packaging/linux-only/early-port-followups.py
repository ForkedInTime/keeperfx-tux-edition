#!/usr/bin/env python3
"""Upstream follow-ups to the team commits a release took early.

    packaging/linux-only/early-port-followups.py [--upstream REF] [--list FILE] [--template] [-v] [REV]

We take some of the team's commits ahead of the weekly sync ("early ports", cherry-picked with -x).
The team sometimes fixes such a commit a few days later. That follow-up only reaches us with the next
sync, so a release cut in between ships the early port's bug without its fix. Stable 1.4.0.6062 to
1.4.0.6098 did exactly that: the team's "creatures stop fighting when keepers ally" (9f192b60e) went
in early on 2026-10-22, and broke lair-enemy fights; the team's fix (a0fb5a095, 2026-10-31) waited for
the 2026-11-05 sync, which stable did not take, and every stable from 3 to 6 November had fights that
stop after half a second.

This lists, for REV (default HEAD): the team commits after REV's last sync that REV carries as early
ports, and every later team commit REV does NOT carry that touches the same C functions (or the same
non-source file), names the port's pull request, or reverts it. Each such pair needs a verdict in the
list file (default: early-port-followups.list next to this script):

    <port sha> <later sha> port|skip <reason>

  port  REV must carry the later commit: it fixes something the early port broke (or the port needs it).
  skip  it does not: a refactor or feature that happens to touch the same code. Say why.

Exit 0 when every pair has a verdict and no 'port' verdict is missing from REV; 1 otherwise (each
problem is printed); 2 on a usage error. --template prints unreviewed pairs as list lines to edit.
Shas in the list may be abbreviated (7+ characters). Needs the team's history as `upstream/master`
(git fetch upstream), and the full history of REV.
"""
import argparse
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SNAPSHOT_KEY = 'Linux-Snapshot-Of'
SOURCE = re.compile(r'\.(c|h|cpp|hpp|cc|hh)$')
NOT_NAMES = {'if', 'for', 'while', 'switch', 'return', 'sizeof', 'defined', 'case', 'else'}


def git(*args, check=True):
    r = subprocess.run(['git', *args], capture_output=True, text=True)
    if check and r.returncode != 0:
        sys.exit(f'early-port-followups: git {" ".join(args)}: {r.stderr.strip()}')
    return r.stdout


# The builtin C/C++ hunk-header driver, so headers name the enclosing function rather than the
# nearest line that starts with a letter.
_attrs = tempfile.NamedTemporaryFile('w', suffix='.gitattributes', delete=False)
_attrs.write(''.join(f'*.{e} diff=cpp\n' for e in ('c', 'h', 'cpp', 'hpp', 'cc', 'hh')))
_attrs.close()


def touched(sha):
    """(functions, non-source files) one commit changes."""
    out = git('-c', f'core.attributesFile={_attrs.name}', 'show', '--format=', '-U0', '--no-color', sha)
    funcs, files, path = set(), set(), None
    for line in out.splitlines():
        if line.startswith('+++ ') or line.startswith('--- '):
            p = line[4:]
            if p != '/dev/null':
                path = p[2:] if p[:2] in ('a/', 'b/') else p
                if not SOURCE.search(path):
                    files.add(path)
        elif line.startswith('@@') and path and SOURCE.search(path):
            header = line.split('@@', 2)[-1]
            for name in re.findall(r'([A-Za-z_]\w*)\s*\(', header):
                if name not in NOT_NAMES:
                    funcs.add(name)
                    break
    return funcs, files


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('rev', nargs='?', default='HEAD')
    ap.add_argument('--upstream', default='upstream/master')
    ap.add_argument('--list', default=os.path.join(HERE, 'early-port-followups.list'))
    ap.add_argument('--template', action='store_true', help='print unreviewed pairs as list lines')
    ap.add_argument('-v', '--verbose', action='store_true', help='also print the pairs that pass')
    a = ap.parse_args()
    for ref in (a.rev, a.upstream):
        if subprocess.run(['git', 'rev-parse', '-q', '--verify', ref + '^{commit}'], capture_output=True).returncode:
            print(f'early-port-followups: unknown revision {ref!r} (fetch the team history as upstream?)', file=sys.stderr)
            return 2

    snap = git('rev-list', '--topo-order', '-n1', f'--grep=^{SNAPSHOT_KEY}: ', a.rev).strip()
    if snap:
        base = [l.split(':', 1)[1].strip() for l in git('log', '-1', '--format=%B', snap).splitlines()
                if l.startswith(SNAPSHOT_KEY + ':')][-1]
    else:
        base = git('merge-base', a.rev, a.upstream).strip()
    team = git('rev-list', '--no-merges', '--topo-order', '--reverse', f'{base}..{a.upstream}').split()
    order = {s: i for i, s in enumerate(team)}

    # What REV carries of those: cherry-picks recorded with -x, and patch-id equivalents.
    carried = set()
    for m in re.finditer(r'cherry picked from commit ([0-9a-f]{40})',
                         git('log', '--no-merges', '--format=%B', '--grep=cherry picked from commit', a.rev)):
        if m.group(1) in order:
            carried.add(m.group(1))
    for line in git('cherry', a.rev, a.upstream, base).splitlines():
        if line.startswith('- '):
            carried.add(line[2:].strip())
    ports = sorted(carried, key=order.get)

    subject = {s: git('log', '-1', '--format=%h %cs %s', s).strip() for s in team}
    body = {s: git('log', '-1', '--format=%B', s) for s in team}
    touch = {s: touched(s) for s in team}

    pairs = []
    for p in ports:
        pfun, pfile = touch[p]
        prs = re.findall(r'\(#(\d+)\)\s*$', subject[p])
        psubj = re.sub(r'^\S+ \S+ ', '', subject[p])
        for later in team[order[p] + 1:]:
            if later in carried:
                continue
            lfun, lfile = touch[later]
            why = []
            shared = sorted(pfun & lfun)
            if shared:
                why.append('functions ' + ', '.join(shared))
            if pfile & lfile:
                why.append('files ' + ', '.join(sorted(pfile & lfile)))
            own = re.findall(r'\(#(\d+)\)\s*$', subject[later])
            if any(re.search(rf'#{n}\b', body[later]) for n in prs if n not in own):
                why.append('names #' + '/#'.join(prs))
            if 'Revert' in body[later] and (psubj in body[later] or p[:7] in body[later]):
                why.append('reverts it')
            if why:
                pairs.append((p, later, '; '.join(why)))

    verdicts = {}
    if os.path.exists(a.list):
        for n, line in enumerate(open(a.list), 1):
            line = line.split('#', 1)[0].strip()
            if not line:
                continue
            f = line.split(None, 3)
            if len(f) < 4 or f[2] not in ('port', 'skip'):
                print(f'{a.list}:{n}: want "<port sha> <later sha> port|skip <reason>"', file=sys.stderr)
                return 2
            verdicts[(f[0], f[1])] = (f[2], f[3])

    def verdict(p, later):
        for (vp, vl), v in verdicts.items():
            if len(vp) >= 7 and len(vl) >= 7 and p.startswith(vp) and later.startswith(vl):
                return v
        return None

    print(f'{a.rev}: last sync {base[:9]}; {len(team)} team commits since, {len(ports)} carried early; '
          f'{len(pairs)} later commit(s) touch an early port and are not carried')
    bad = 0
    for p, later, why in pairs:
        v = verdict(p, later)
        if v is None:
            bad += 1
            print(f'UNREVIEWED  {subject[later]}\n    after early port {subject[p]}\n    shares {why}')
        elif v[0] == 'port':
            bad += 1
            print(f'MISSING     {subject[later]}\n    after early port {subject[p]}\n    verdict port: {v[1]}')
        elif a.verbose:
            print(f'ok (skip)   {subject[later]}\n    after early port {subject[p]}: {v[1]}')
    if a.template:
        for p, later, why in pairs:
            if verdict(p, later) is None:
                print(f'{p[:12]} {later[:12]} skip|port  # {subject[later].split(" ", 1)[1]} '
                      f'-- after {subject[p].split(" ", 1)[1]}; {why}')
    if bad:
        print(f'{bad} problem(s). A "port" commit must be in {a.rev} before it ships; give every other pair a '
              f'verdict in {os.path.relpath(a.list)} (see this script\'s header).')
        return 1
    print('ok')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    finally:
        os.unlink(_attrs.name)
