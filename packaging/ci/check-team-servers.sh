#!/usr/bin/env bash
#
# Fail if the engine points anything at the KeeperFX team's servers.
#
#   packaging/ci/check-team-servers.sh
#
# KeeperFX: Tux Edition sends nothing to keeperfx.net or the team's Cloudflare
# workers: online games go through our own lobby (lobby-server/), and crash
# reports come to our issue tracker. Upstream code is still merged every week,
# so a sync that brings one of their addresses back -- a new default in a
# config file, a new URL in the source -- fails here instead of shipping.
#
# What counts: a non-comment line of a .cfg file under config/, or a quoted
# string in src/, that names keeperfx.net or keeperfx.workers.dev. Comments
# and readmes that only mention the sites are fine. The one allowed string is
# src/net_matchmaking.c's list of retired domains, which exists to replace
# them with ours.
#
# Exit status: 0 clean, 1 an address of the team's servers was found.
set -euo pipefail
cd "$(dirname "$0")/../.."

domains='keeperfx\.(workers\.dev|net)'

found=0
report() {
    found=1
    printf '  %s\n' "$@"
}

cfg_hits="$(git grep -nIE "$domains" -- 'config/*.cfg' 'config/**/*.cfg' \
    | grep -vE '^[^:]+:[0-9]+:[[:space:]]*[;#]' || true)"
if [ -n "$cfg_hits" ]; then
    echo "Config settings that name the team's servers:"
    mapfile -t lines <<<"$cfg_hits"
    report "${lines[@]}"
fi

src_hits="$(git grep -nIE "\"[^\"]*$domains[^\"]*\"" -- src \
    | grep -vE '^src/net_matchmaking\.c:[0-9]+:static const char \*const retired_domains\[\]' || true)"
if [ -n "$src_hits" ]; then
    echo "Source strings that name the team's servers:"
    mapfile -t lines <<<"$src_hits"
    report "${lines[@]}"
fi

if [ "$found" -ne 0 ]; then
    echo
    echo "This edition sends nothing to the KeeperFX team's servers. Point the setting at"
    echo "our own service (the lobby: lobby-server/), or drop the feature."
    exit 1
fi
echo "No address of the KeeperFX team's servers in config/ or src/."
