// Protocol test for the lobby server, speaking exactly as the game does.
//
// Every message below is built the way src/net_matchmaking.c and
// src/bflib_netsession.c build theirs, so a change on either side that breaks
// the other shows up here. Runs against a local server:
//
//   npx wrangler dev --port 8799 --var HOST_SILENCE_MS:3000 --var SWEEP_INTERVAL_MS:1000
//   LOBBY_URL=http://127.0.0.1:8799 npm test
//
// Uses the WebSocket and fetch built into Node 22 and later.

import assert from "node:assert/strict";

const base = process.env.LOBBY_URL ?? "http://127.0.0.1:8799";
const wsUrl = base.replace(/^http/, "ws") + "/ws";
const VERSION = "1.4.0.6088";
const VER_STRING = `${VERSION} alpha`;

// net_session_metadata_json()
function metadata({ name = "Test game", version = VERSION, phase = "Lobby", joinable = true, maxPlayers = 4, spectatorsEnabled = false, spectatorCount = 0, rejoinOpen = false, players = ["Host"] } = {}) {
	return `"name":"${name}","version":"${version}","phase":"${phase}","joinable":${joinable},"maxPlayers":${maxPlayers},"spectatorSupport":1,"spectatorsEnabled":${spectatorsEnabled},"maxSpectators":50,"spectatorCount":${spectatorCount},"rejoinOpen":${rejoinOpen},"players":[${players.map((p) => `"${p}"`).join(",")}]`;
}

// matchmaking_create_thread(); a host listing its game again after losing the
// lobby connection adds the id and key of the listing it lost.
const create = (meta, { ipv4Port = 5556, ipv6Port = 5556, direct = 5557, ipv4 = "203.0.113.10", ipv6 = "", replaces, key } = {}) =>
	`{"action":"create","ipv4Port":${ipv4Port},"ipv6Port":${ipv6Port},"directIpv4Port":${direct},${meta},"ipv4":"${ipv4}","ipv6":"${ipv6}","resultActions":true` +
	(replaces ? `,"replaces":"${replaces}","key":"${key}"` : "") +
	"}";
const update = (id, meta) => `{"action":"update","id":"${id}",${meta}}`;
const started = (id, meta) => `{"action":"game_started","id":"${id}","mapNumber":1,"mapName":"Map 1",${meta}}`;
const punch = (lobbyId, { version = VERSION, role = "player", ipv4 = "198.51.100.20", ipv4Port = 6000 } = {}) =>
	`{"action":"punch","lobbyId":"${lobbyId}","myIpv4Port":${ipv4Port},"myIpv6Port":0,"myIpv4":"${ipv4}","myIpv6":"","version":"${version}","role":"${role}","spectatorSupport":1}`;
const LIST = `{"action":"list","version":"${VER_STRING}"}`;

// Every connection a test opens, so it is closed even when the test fails and
// cannot leave a game listed for the next one.
const openClients = new Set();

class Client {
	constructor(name) {
		this.name = name;
		this.inbox = [];
		this.waiters = [];
		this.closed = false;
	}

	async open() {
		this.ws = new WebSocket(wsUrl);
		openClients.add(this);
		this.ws.addEventListener("message", (event) => {
			const message = JSON.parse(event.data);
			const waiter = this.waiters.find((w) => w.type === message.type);
			if (waiter) {
				this.waiters.splice(this.waiters.indexOf(waiter), 1);
				waiter.resolve(message);
			} else {
				this.inbox.push(message);
			}
		});
		this.ws.addEventListener("close", () => (this.closed = true));
		await new Promise((resolve, reject) => {
			this.ws.addEventListener("open", resolve, { once: true });
			this.ws.addEventListener("error", reject, { once: true });
		});
		return this;
	}

	send(text) {
		this.ws.send(text);
	}

	/** The next message of this type, from what already arrived or what comes. */
	next(type, timeoutMs = 3000) {
		const index = this.inbox.findIndex((m) => m.type === type);
		if (index >= 0) {
			return Promise.resolve(this.inbox.splice(index, 1)[0]);
		}
		return new Promise((resolve, reject) => {
			const waiter = { type, resolve };
			this.waiters.push(waiter);
			setTimeout(() => {
				const at = this.waiters.indexOf(waiter);
				if (at >= 0) {
					this.waiters.splice(at, 1);
					reject(new Error(`${this.name}: no "${type}" within ${timeoutMs} ms (inbox: ${JSON.stringify(this.inbox)})`));
				}
			}, timeoutMs);
		});
	}

	/** Waits for the newest list, skipping older ones already queued. */
	async latestList(timeoutMs = 3000) {
		let list = await this.next("lobbies", timeoutMs);
		for (;;) {
			const index = this.inbox.findIndex((m) => m.type === "lobbies");
			if (index < 0) {
				return list;
			}
			list = this.inbox.splice(index, 1)[0];
		}
	}

	/** Waits for a list that satisfies `wanted`, skipping lists sent before it. */
	async listWhere(wanted, timeoutMs = 3000) {
		const deadline = Date.now() + timeoutMs;
		for (;;) {
			const list = await this.next("lobbies", Math.max(1, deadline - Date.now()));
			if (wanted(list.lobbies)) {
				return list;
			}
		}
	}

	close() {
		openClients.delete(this);
		this.ws.close();
	}
}

/** Closes what a test left open and waits until the server has seen it. */
async function closeLeftovers() {
	if (openClients.size === 0) {
		return;
	}
	const leftovers = [...openClients];
	leftovers.forEach((client) => client.close());
	for (let i = 0; i < 50 && leftovers.some((client) => !client.closed); i++) {
		await sleep(100);
	}
	await sleep(300);
}

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
const results = [];
async function test(name, body) {
	try {
		await body();
		results.push([name, true]);
		console.log(`ok   ${name}`);
	} catch (error) {
		results.push([name, false]);
		console.log(`FAIL ${name}\n     ${error.stack ?? error}`);
	} finally {
		await closeLeftovers();
	}
}

// Give a server that is still starting up half a minute.
for (let attempt = 0; ; attempt++) {
	try {
		await fetch(`${base}/`);
		break;
	} catch (error) {
		if (attempt >= 60) {
			console.log(`No lobby server at ${base}: ${error.cause?.code ?? error}`);
			process.exit(1);
		}
		await sleep(500);
	}
}

await test("/ip answers with a bare address, no newline", async () => {
	const response = await fetch(`${base}/ip`);
	const text = await response.text();
	assert.equal(response.status, 200);
	assert.ok(text.length > 0, "empty address");
	assert.ok(!/\s/.test(text), `whitespace in ${JSON.stringify(text)}`);
});

await test("an empty lobby lists nothing", async () => {
	const browser = await new Client("browser").open();
	browser.send(LIST);
	const list = await browser.next("lobbies");
	assert.deepEqual(list.lobbies, []);
	browser.close();
});

await test("host, list, join, start, close: the whole round", async () => {
	const browser = await new Client("browser").open();
	const host = await new Client("host").open();
	browser.send(LIST);
	await browser.next("lobbies");

	host.send(create(metadata()));
	const created = await host.next("created");
	assert.match(created.id, /^[0-9a-f]{32}$/);

	const listed = await browser.latestList();
	assert.equal(listed.lobbies.length, 1);
	const game = listed.lobbies[0];
	assert.equal(game.id, created.id);
	assert.equal(game.name, "Test game");
	assert.equal(game.version, VERSION);
	assert.deepEqual(game.players, ["Host"]);
	for (const key of ["ipv4", "ipv6", "ipv4Port", "ipv6Port", "directIpv4Port"]) {
		assert.ok(!(key in game), `the public list leaks ${key}`);
	}
	assert.ok(!JSON.stringify(listed).includes("203.0.113.10"), "host address in the public list");

	// Heartbeat.
	host.send('{"action":"ping"}');
	await host.next("pong");

	// A player joins: both sides learn the other's address.
	browser.send(punch(created.id));
	const toJoiner = await browser.next("punch");
	// The host reported no IPv6 address, so the server offers the one its
	// connection came from -- none when the test reached it over IPv4, as the
	// game always does (it opens the lobby connection over IPv4 only).
	const { peerIpv6, ...rest } = toJoiner;
	assert.ok(peerIpv6 === "" || peerIpv6.includes(":"), `odd IPv6 ${peerIpv6}`);
	assert.deepEqual(rest, { type: "punch", peerIpv4: "203.0.113.10", peerIpv4Port: 5556, peerIpv6Port: 5556, peerDirectIpv4Port: 5557 });
	const toHost = await host.next("punch");
	assert.deepEqual(toHost, { type: "punch", peerIpv4: "198.51.100.20", peerIpv6: "", peerIpv4Port: 6000, peerIpv6Port: 0 });

	// The roster grows.
	host.send(update(created.id, metadata({ players: ["Host", "Guest"] })));
	assert.deepEqual((await browser.latestList()).lobbies[0].players, ["Host", "Guest"]);

	// The match starts: no more players, and it stays started.
	host.send(started(created.id, metadata({ phase: "Loading", joinable: false, players: ["Host", "Guest"] })));
	assert.deepEqual(await host.next("game_started"), { type: "game_started", success: true });
	const startedList = await browser.latestList();
	assert.equal(startedList.lobbies[0].phase, "In-Game");
	assert.equal(startedList.lobbies[0].joinable, false);
	host.send(update(created.id, metadata({ phase: "Loading", joinable: false, players: ["Host", "Guest"] })));
	assert.equal((await browser.latestList()).lobbies[0].phase, "In-Game");

	browser.send(punch(created.id));
	const late = await browser.next("error");
	assert.equal(late.reason, 1);

	// Spectators: refused while the host has them off, let in once it allows them.
	browser.send(punch(created.id, { role: "spectator" }));
	assert.equal((await browser.next("error")).reason, 6);
	host.send(update(created.id, metadata({ phase: "In-Game", joinable: false, spectatorsEnabled: true, players: ["Host", "Guest"] })));
	await browser.latestList();
	browser.send(punch(created.id, { role: "spectator" }));
	await browser.next("punch");
	await host.next("punch");

	// The host leaves.
	host.send(`{"action":"delete","id":"${created.id}"}`);
	assert.deepEqual(await host.next("deleted"), { type: "deleted", success: true });
	assert.deepEqual((await browser.latestList()).lobbies, []);
	host.close();
	browser.close();
});

await test("a host without an IPv6 port is never offered over IPv6", async () => {
	const browser = await new Client("browser").open();
	const host = await new Client("host").open();
	host.send(create(metadata({ name: "IPv4 only" }), { ipv6Port: 0 }));
	const { id } = await host.next("created");
	browser.send(punch(id));
	const toJoiner = await browser.next("punch");
	assert.equal(toJoiner.peerIpv6, "");
	assert.equal(toJoiner.peerIpv4, "203.0.113.10");
	host.send(`{"action":"cancel","id":"${id}"}`);
	await host.next("deleted");
});

await test("a different build is refused with the version reason", async () => {
	const browser = await new Client("browser").open();
	const host = await new Client("host").open();
	host.send(create(metadata()));
	const { id } = await host.next("created");
	browser.send(punch(id, { version: "1.4.0.6087" }));
	const refused = await browser.next("error");
	assert.equal(refused.reason, 4);
	host.send(`{"action":"cancel","id":"${id}"}`);
	await host.next("deleted");
	host.close();
	browser.close();
});

await test("a full or locked game is refused", async () => {
	const browser = await new Client("browser").open();
	const host = await new Client("host").open();
	host.send(create(metadata({ maxPlayers: 2, players: ["Host", "Guest"] })));
	const { id } = await host.next("created");
	browser.send(punch(id));
	assert.equal((await browser.next("error")).reason, 3);
	host.send(update(id, metadata({ joinable: false, players: ["Host"] })));
	await sleep(200);
	browser.send(punch(id));
	assert.equal((await browser.next("error")).reason, 2);
	host.send(`{"action":"cancel","id":"${id}"}`);
	await host.next("deleted");
	host.close();
	browser.close();
});

await test("only the host can change or close its game", async () => {
	const browser = await new Client("browser").open();
	const host = await new Client("host").open();
	host.send(create(metadata()));
	const { id } = await host.next("created");
	browser.send(update(id, metadata({ name: "Stolen" })));
	await browser.next("error");
	browser.send(`{"action":"delete","id":"${id}"}`);
	assert.deepEqual(await browser.next("deleted"), { type: "deleted", success: false });
	browser.send(LIST);
	const list = await browser.latestList();
	assert.equal(list.lobbies.length, 1);
	assert.equal(list.lobbies[0].name, "Test game");
	host.close();
	browser.close();
});

await test("a host that disconnects disappears from the list", async () => {
	const browser = await new Client("browser").open();
	const host = await new Client("host").open();
	host.send(create(metadata({ name: "Leaving" })));
	await host.next("created");
	browser.send(LIST);
	await browser.listWhere((lobbies) => lobbies.some((l) => l.name === "Leaving"));
	host.close();
	await browser.listWhere((lobbies) => !lobbies.some((l) => l.name === "Leaving"));
	browser.close();
});

await test("a listed game gets a private key that the list never shows", async () => {
	const browser = await new Client("browser").open();
	const host = await new Client("host").open();
	host.send(create(metadata({ name: "Keyed" })));
	const created = await host.next("created");
	assert.match(created.key ?? "", /^[0-9a-f]{32}$/, `no key in ${JSON.stringify(created)}`);
	assert.notEqual(created.key, created.id);
	browser.send(LIST);
	const list = await browser.listWhere((lobbies) => lobbies.some((l) => l.name === "Keyed"));
	assert.ok(!JSON.stringify(list).includes(created.key), "the key is in the public list");
	host.close();
	browser.close();
});

// The host's old connection can stay open on the server's side after the game has
// lost it (no goodbye reached the server). The game then lists itself again on a new
// connection, naming the old listing and its key: one game, listed once, at once.
await test("a host listing its game again replaces its stale listing at once", async () => {
	const browser = await new Client("browser").open();
	const stale = await new Client("stale connection").open();
	stale.send(create(metadata({ name: "Relisted" })));
	const first = await stale.next("created");
	// To the server a stale connection looks alive until the silence limit (3 s in
	// the test server): keep it pinging, so only the replacement can remove it.
	const keepAlive = setInterval(() => !stale.closed && stale.send('{"action":"ping"}'), 500);
	try {
		browser.send(LIST);
		await browser.listWhere((lobbies) => lobbies.some((l) => l.id === first.id));
		await sleep(1000);

		const fresh = await new Client("new connection").open();
		fresh.send(create(metadata({ name: "Relisted" }), { replaces: first.id, key: first.key }));
		const second = await fresh.next("created");
		assert.notEqual(second.id, first.id);
		const list = await browser.listWhere((lobbies) => !lobbies.some((l) => l.id === first.id), 1500);
		assert.deepEqual(
			list.lobbies.filter((l) => l.name === "Relisted").map((l) => l.id),
			[second.id],
		);
		for (let i = 0; i < 10 && !stale.closed; i++) {
			await sleep(100);
		}
		assert.ok(stale.closed, "the stale connection was left open");

		// The new listing is a normal one: it can be joined.
		browser.send(punch(second.id));
		await browser.next("punch");
		await fresh.next("punch");
		fresh.close();
		browser.close();
	} finally {
		clearInterval(keepAlive);
	}
});

await test("a wrong key or another game's key replaces nothing", async () => {
	const browser = await new Client("browser").open();
	const victim = await new Client("victim").open();
	victim.send(create(metadata({ name: "Victim" })));
	const target = await victim.next("created");
	const other = await new Client("other host").open();
	other.send(create(metadata({ name: "Other" })));
	const own = await other.next("created");
	// Out of reach of the test server's 3-second silence limit.
	const keepAlive = setInterval(() => [victim, other].forEach((c) => !c.closed && c.send('{"action":"ping"}')), 500);
	try {
		const guesser = await new Client("guesser").open();
		guesser.send(create(metadata({ name: "Guess" }), { replaces: target.id, key: "0".repeat(32) }));
		await guesser.next("created");
		const thief = await new Client("thief").open();
		thief.send(create(metadata({ name: "Thief" }), { replaces: target.id, key: own.key }));
		await thief.next("created");

		browser.send(LIST);
		const list = await browser.listWhere((lobbies) => ["Guess", "Thief"].every((name) => lobbies.some((l) => l.name === name)));
		assert.ok(list.lobbies.some((l) => l.id === target.id), "the victim's game was removed");
		assert.ok(list.lobbies.some((l) => l.id === own.id), "the other host's game was removed");
		await sleep(300);
		assert.ok(!victim.closed, "the victim's connection was closed");
		guesser.close();
		thief.close();
	} finally {
		clearInterval(keepAlive);
	}
	victim.close();
	other.close();
	browser.close();
});

// Needs the test server's 3-second silence limit; the deployed one waits 100 s.
const local = /^http:\/\/(127\.0\.0\.1|localhost)[:/]/.test(base);
await (local ? test : (name) => console.log(`skip ${name} (needs the local test server)`))("a host that goes quiet is dropped; one that keeps pinging stays", async () => {
	const browser = await new Client("browser").open();
	const quiet = await new Client("quiet host").open();
	const alive = await new Client("pinging host").open();
	quiet.send(create(metadata({ name: "Quiet" })));
	await quiet.next("created");
	alive.send(create(metadata({ name: "Alive" })));
	await alive.next("created");
	// HOST_SILENCE_MS is 3000 in the test server; ping well inside that.
	for (let i = 0; i < 6; i++) {
		alive.send('{"action":"ping"}');
		await alive.next("pong");
		await sleep(1000);
	}
	browser.send(LIST);
	const names = (await browser.latestList()).lobbies.map((l) => l.name);
	assert.ok(!names.includes("Quiet"), `quiet host still listed: ${names}`);
	assert.ok(names.includes("Alive"), `pinging host dropped: ${names}`);
	await sleep(300);
	assert.ok(quiet.closed, "quiet host's connection left open");
	alive.close();
	browser.close();
});

await test("junk is refused without breaking the connection", async () => {
	const client = await new Client("client").open();
	client.send("not json");
	await client.next("error");
	client.send('{"action":"nonsense"}');
	await client.next("error");
	client.send(LIST);
	await client.next("lobbies");
	client.close();
});

const failed = results.filter(([, ok]) => !ok).length;
console.log(`\n${results.length - failed}/${results.length} passed`);
process.exit(failed ? 1 : 0);
