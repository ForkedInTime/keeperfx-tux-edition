# Lobby server

The online lobby for KeeperFX: Tux Edition: the list of internet games in the
game's Multiplayer menu, and the introduction that lets two players behind home
routers reach each other. It runs on Cloudflare Workers as one small Durable
Object.

The KeeperFX team runs a lobby of its own for their Windows builds; this edition
does not use it. Our builds cannot join their games anyway, since a join needs
exactly the same build, and the game sends their lobby nothing. A keeperfx.cfg
that still names their lobby is pointed at this one by the engine and by the
launcher.

## What it does

The game keeps one WebSocket open to `wss://<host>/ws` and speaks the protocol
in [`src/net_matchmaking.c`](../src/net_matchmaking.c):

| The game sends | It means | The server answers |
| --- | --- | --- |
| `list` | Show me the games | `lobbies`: every open game's name, version, phase and players |
| `create` | I am hosting | `created` with the game's id; everyone browsing gets the new list |
| `update`, `game_started` | My game changed / has started | the new list to everyone browsing; `game_started` is confirmed |
| `cancel`, `delete` | I stopped hosting | `deleted`; the game leaves the list |
| `punch` | I want to join this game | the host's address to the player, and the player's to the host |
| `ping` | I am still hosting (every 30 s) | `pong` |

`https://<host>/ip` answers with the address the request came from. The game
uses it to learn its own public addresses.

Joining only swaps addresses. Both games then send each other a few packets
through their routers (UDP hole punching), and from then on the game runs
directly between the players, never through this server.

A join is refused with the reason the game shows when the builds differ, the
game has started, is locked or full, or spectators are not allowed. A game whose
host stops pinging is dropped after 100 seconds.

A host that loses its connection lists its game again by itself. The new
listing names the old one and the key the server gave the host for it (never
listed, never logged), and the old row goes at once, even when the server
never saw the old connection end. Without that, the game would show twice until
the old one went quiet.

## What it keeps

Nothing beyond the open connections, apart from a short event log. No storage,
no messages to anyone else.

The log exists to explain dropped connections. It has one line per event (a
game listed, closed or listed again, a connection ending, a host going quiet,
the server starting or waking up), with the game's random id, the build number,
a few numbers such as how many connections are open, and how a connection
ended. It never holds an address, a player or game name, a host's key, or any
other text a game sends, and Cloudflare's own request logs are off
(`observability` in `wrangler.jsonc`). Cloudflare keeps these lines for a few
days (3 on the free plan, 7 on the paid one).

The public list carries no addresses. A host's address goes only to a player
who asked to join that host, and the player's address only to that host. That
is the least either of them needs to connect.

## Deploying

The first time, sign in to the Cloudflare account that will run it:

```sh
cd lobby-server
npm ci
npx wrangler login
```

Then, after every change:

```sh
npm run check     # TypeScript
npm run deploy
```

Deploying prints the server's address, `lobby.<account>.workers.dev`.
That host name is the default in three places, which must match it:
`MATCHMAKING_HOST_DEFAULT` in `src/net_matchmaking.c`, `MATCHMAKING_SERVER` in
`config/keeperfx.cfg`, and `LobbyServer::DEFAULT_HOST` in the launcher's
`src/lobbyserver.h`.

A deploy restarts the server and closes every connection. Hosting games drop
off the list until their hosts host again, so deploy when nobody is playing.

## Testing

`test/protocol.test.mjs` speaks to a local copy of the server the way the game
does. It covers hosting, listing, joining (players and spectators), refusals,
leaving, disconnecting and silent hosts:

```sh
npx wrangler dev --port 8799 --var HOST_SILENCE_MS:3000 --var SWEEP_INTERVAL_MS:1000
LOBBY_URL=http://127.0.0.1:8799 npm test      # in a second terminal
```

The two `--var`s shorten the silent-host timeout so that test takes seconds.
Point `LOBBY_URL` at the deployed server (`https://<host>`) to run the rest of
the test there; the silent-host test is skipped, since it needs the shortened
timeout.

When an upstream sync changes `src/net_matchmaking.c` or the lobby metadata in
`src/bflib_netsession.c`, compare the messages with this server and the test,
and update both.
