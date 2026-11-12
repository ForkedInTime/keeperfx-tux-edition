/**
 * Lobby server for KeeperFX: Tux Edition online multiplayer.
 *
 * The game (src/net_matchmaking.c) keeps one WebSocket open to /ws. A host
 * advertises its game here, players list the games and ask to join one, and
 * joining only swaps the two players' internet addresses so that both can open
 * their routers towards each other (UDP hole punching). The game itself then
 * runs directly between the players and never passes through this server.
 * /ip answers a game asking which address it is seen from.
 *
 * On purpose:
 *  - nothing is written to storage, and nothing is sent anywhere else;
 *  - the only log is a short line per connection event (see log()), so that a
 *    dropped connection can be explained: never an address, a name or a key;
 *  - the public list never carries an address: a host's address goes only to a
 *    player who asked to join that host, and the player's only to that host.
 *
 * Everything lives in one Durable Object so that every connection sees the
 * same list. Each connection keeps its own state in its WebSocket attachment,
 * which survives the object sleeping between messages (hibernation).
 */
import { DurableObject } from "cloudflare:workers";

export interface Env {
	LOBBY: DurableObjectNamespace<Lobby>;
	/** Test hook: milliseconds of host silence before its game is dropped. */
	HOST_SILENCE_MS?: string;
	/** Test hook: milliseconds between checks for silent hosts. */
	SWEEP_INTERVAL_MS?: string;
}

/** Why a join was refused: the game's enum NetJoinRejection (src/bflib_netsession.h). */
const Refused = {
	InGame: 1,
	Locked: 2,
	Full: 3,
	Version: 4,
	SpectatorState: 5,
	SpectatorsDisabled: 6,
	SpectatorsFull: 7,
	SpectatorsUnsupported: 8,
} as const;

/** The game's session_phase_names (src/bflib_netsession.c). */
const PHASES = ["Unknown", "Lobby", "In-Game", "In-Landview", "Loading"] as const;
type Phase = (typeof PHASES)[number];

// Limits follow the game's own buffers (src/bflib_netsession.h, src/net_main.h).
const MAX_GAMES = 100; // the game shows at most 32 (MATCHMAKING_SESSIONS_MAX)
const MAX_PLAYERS = 4; // SESSION_HUMANS_MAX
const PLAYER_NAME_MAX_BYTES = 31; // NETSP_PLAYER_NAME_MAX_LEN, less the terminator
const GAME_NAME_MAX_CHARS = 64;
const VERSION_MAX_CHARS = 32;
const MAX_SPECTATORS = 50; // MAX_NET_SPECTATORS
const MAX_MESSAGE_CHARS = 4096; // the game's SEND_BUFFER_SIZE

// A host pings every 30 s (HEARTBEAT_INTERVAL_MS) and gives up after three
// unanswered pings, so a host silent for well over a minute is gone.
const HOST_SILENCE_MS = 100_000;
const SWEEP_INTERVAL_MS = 30_000;

// The host's heartbeat is answered without waking the object.
const PING = '{"action":"ping"}';
const PONG = '{"type":"pong"}';

const encoder = new TextEncoder();

/** A game as everyone browsing sees it. */
interface Game {
	name: string;
	version: string;
	phase: Phase;
	joinable: boolean;
	maxPlayers: number;
	players: string[];
	spectatorSupport: number;
	spectatorsEnabled: boolean;
	maxSpectators: number;
	spectatorCount: number;
	rejoinOpen: boolean;
	createdAt: number;
}

/** How to reach a host. Never listed; given only to a player joining it. */
interface Route {
	ipv4: string;
	ipv6: string;
	ipv4Port: number;
	ipv6Port: number;
	directIpv4Port: number;
}

/** What one connection carries in its WebSocket attachment. */
interface Peer {
	/** The address the connection came from. */
	ip: string;
	gameId?: string;
	game?: Game;
	route?: Route;
	/**
	 * Given only to the host, so that it can replace this listing when it lists
	 * the game again after losing its connection. Never listed or logged.
	 */
	key?: string;
	/** When the host last sent anything other than a ping (ms). */
	heardAt?: number;
}

type Message = Record<string, unknown> & { action: string };

/**
 * One line in the server's log (Workers Logs, kept a few days by Cloudflare).
 * Only what explains a dropped connection: what happened, to which game (the
 * same random id the public list shows) and how many are connected. Never an
 * address, a player or game name, or a key.
 */
function log(event: string, fields: Record<string, string | number | boolean> = {}): void {
	console.log({ event, ...fields });
}

/** A game's id as the log shows it: enough to follow one game. */
function shortId(id: string | undefined): string {
	return (id ?? "").slice(0, 8);
}

/** A build number for the log, or "other": the version is whatever the client sent. */
function logVersion(version: string): string {
	const build = firstWord(version);
	return /^\d{1,5}(\.\d{1,6}){1,3}$/.test(build) ? build : "other";
}

function sameKey(a: string, b: string): boolean {
	const left = encoder.encode(a);
	const right = encoder.encode(b);
	return left.byteLength === right.byteLength && crypto.subtle.timingSafeEqual(left, right);
}

function isMessage(value: unknown): value is Message {
	return typeof value === "object" && value !== null && !Array.isArray(value) && typeof (value as Message).action === "string";
}

function port(value: unknown): number {
	return Number.isInteger(value) && (value as number) >= 1 && (value as number) <= 65535 ? (value as number) : 0;
}

function int(value: unknown, min: number, max: number): number | undefined {
	return Number.isInteger(value) && (value as number) >= min && (value as number) <= max ? (value as number) : undefined;
}

function ipv4(value: unknown): string {
	if (typeof value !== "string") {
		return "";
	}
	const parts = value.split(".");
	if (parts.length !== 4 || !parts.every((part) => /^\d{1,3}$/.test(part) && Number(part) <= 255)) {
		return "";
	}
	return value;
}

function ipv6(value: unknown): string {
	if (typeof value !== "string" || value.length > 45 || !value.includes(":") || !/^[0-9a-fA-F:.]+$/.test(value)) {
		return "";
	}
	try {
		return new URL(`http://[${value}]/`).hostname.slice(1, -1);
	} catch {
		return "";
	}
}

/**
 * A peer's addresses: the ones it reports win, because the game learns its
 * IPv4 from STUN on its own UDP socket, which is the address that matters for
 * the game; the address the connection came from fills any gap.
 */
function addresses(peer: Peer, reportedIpv4: unknown, reportedIpv6: unknown): { ipv4: string; ipv6: string } {
	const seenIpv4 = ipv4(peer.ip);
	const seenIpv6 = seenIpv4 ? "" : ipv6(peer.ip);
	return { ipv4: ipv4(reportedIpv4) || seenIpv4, ipv6: ipv6(reportedIpv6) || seenIpv6 };
}

function firstWord(value: unknown): string {
	return typeof value === "string" ? (value.trim().split(/\s+/, 1)[0] ?? "") : "";
}

function playerName(value: unknown): value is string {
	return typeof value === "string" && value.length > 0 && encoder.encode(value).length <= PLAYER_NAME_MAX_BYTES && !/[\u0000-\u001f]/.test(value);
}

function newGame(): Game {
	return {
		name: "Unknown",
		version: "",
		phase: "Lobby",
		joinable: true,
		maxPlayers: 0,
		players: [],
		spectatorSupport: 0,
		spectatorsEnabled: false,
		maxSpectators: 0,
		spectatorCount: 0,
		rejoinOpen: false,
		createdAt: Date.now(),
	};
}

/**
 * Takes what the host says about its game. A field that does not make sense is
 * skipped rather than refused: an error ends the host's connection, and one odd
 * field is no reason to pull a game off the list.
 */
function readGame(message: Message, game: Game): void {
	if (typeof message.name === "string" && message.name.length > 0) {
		game.name = [...message.name].slice(0, GAME_NAME_MAX_CHARS).join("");
	}
	if (typeof message.version === "string") {
		game.version = message.version.trim().slice(0, VERSION_MAX_CHARS);
	}
	// Once started, a game stays started: the host leaves the lobby when it ends.
	if (typeof message.phase === "string" && (PHASES as readonly string[]).includes(message.phase) && game.phase !== "In-Game") {
		game.phase = message.phase as Phase;
	}
	if (typeof message.joinable === "boolean") {
		game.joinable = message.joinable;
	}
	const maxPlayers = int(message.maxPlayers, 1, MAX_PLAYERS);
	if (maxPlayers !== undefined) {
		game.maxPlayers = maxPlayers;
	}
	if (Array.isArray(message.players) && message.players.length <= MAX_PLAYERS && message.players.every(playerName)) {
		game.players = message.players;
	}
	if (message.spectatorSupport === 0 || message.spectatorSupport === 1) {
		game.spectatorSupport = message.spectatorSupport;
	}
	if (typeof message.spectatorsEnabled === "boolean") {
		game.spectatorsEnabled = message.spectatorsEnabled;
	}
	const maxSpectators = int(message.maxSpectators, 0, MAX_SPECTATORS);
	if (maxSpectators !== undefined) {
		game.maxSpectators = maxSpectators;
	}
	const spectatorCount = int(message.spectatorCount, 0, MAX_SPECTATORS);
	if (spectatorCount !== undefined) {
		game.spectatorCount = spectatorCount;
	}
	if (typeof message.rejoinOpen === "boolean") {
		game.rejoinOpen = message.rejoinOpen;
	}
	if (game.phase === "In-Game") {
		game.joinable = false;
	}
}

/** Why a player cannot join this game, or nothing if they can. */
function playerRefusal(game: Game): [string, number] | undefined {
	if (game.phase === "In-Game") {
		return ["The game has already started.", Refused.InGame];
	}
	if (!game.joinable) {
		return ["Joining is locked for the moment.", Refused.Locked];
	}
	if (game.maxPlayers && game.players.length >= game.maxPlayers) {
		return ["The game is full.", Refused.Full];
	}
	return undefined;
}

/** Why a spectator cannot watch this game, or nothing if they can. */
function spectatorRefusal(game: Game, version: string, hostVersion: string): [string, number] | undefined {
	if (game.spectatorSupport !== 1) {
		return ["This host does not take spectators.", Refused.SpectatorsUnsupported];
	}
	if (!version || !hostVersion) {
		return ["Watching needs the same game version.", Refused.Version];
	}
	if (game.phase !== "In-Game") {
		return ["Spectators can only join a game that has started.", Refused.SpectatorState];
	}
	if (!game.spectatorsEnabled && !game.rejoinOpen) {
		return ["The host has turned spectators off.", Refused.SpectatorsDisabled];
	}
	if (!game.maxSpectators || game.spectatorCount >= game.maxSpectators) {
		return ["There is no room for another spectator.", Refused.SpectatorsFull];
	}
	return undefined;
}

export class Lobby extends DurableObject<Env> {
	constructor(ctx: DurableObjectState, env: Env) {
		super(ctx, env);
		ctx.setWebSocketAutoResponse(new WebSocketRequestResponsePair(PING, PONG));
		// Runs on a fresh start and on every wake from hibernation, so it is the
		// commonest line. A restart by Cloudflare ends every connection: it shows
		// here as a start with none left.
		log("lobby_started", { connections: ctx.getWebSockets().length });
	}

	async fetch(request: Request): Promise<Response> {
		if (request.headers.get("Upgrade")?.toLowerCase() !== "websocket") {
			return new Response("Expected a WebSocket\n", { status: 426 });
		}
		const pair = new WebSocketPair();
		const [client, server] = [pair[0], pair[1]];
		this.ctx.acceptWebSocket(server);
		const peer: Peer = { ip: request.headers.get("CF-Connecting-IP") ?? "" };
		server.serializeAttachment(peer);
		return new Response(null, { status: 101, webSocket: client });
	}

	async webSocketMessage(ws: WebSocket, raw: string | ArrayBuffer): Promise<void> {
		if (typeof raw !== "string") {
			return;
		}
		let message: unknown;
		try {
			message = raw.length <= MAX_MESSAGE_CHARS ? JSON.parse(raw) : undefined;
		} catch {
			message = undefined;
		}
		if (!isMessage(message)) {
			return this.refuse(ws, "Message not understood");
		}
		const peer = this.peer(ws);
		if (peer.game) {
			peer.heardAt = Date.now();
			ws.serializeAttachment(peer);
		}
		switch (message.action) {
			case "list":
				return this.send(ws, this.list());
			case "create":
				return this.create(ws, peer, message);
			case "update":
			case "game_started":
				return this.update(ws, peer, message);
			case "cancel":
			case "delete":
				return this.close(ws, peer, message);
			case "punch":
				return this.punch(ws, message);
			case "ping":
				// Normally answered by the auto-response; this catches any other spelling.
				return this.send(ws, PONG);
			case "pong":
				return;
			default:
				return this.refuse(ws, "Unknown action");
		}
	}

	async webSocketClose(ws: WebSocket, code: number, _reason: string, wasClean: boolean): Promise<void> {
		this.hangUp(ws, { code, clean: wasClean });
	}

	async webSocketError(ws: WebSocket, error: unknown): Promise<void> {
		// The kind of error only: its message is text from outside this code.
		this.hangUp(ws, { error: error instanceof Error ? error.name : "unknown" });
	}

	/** Drops the games of hosts that have gone quiet without saying goodbye. */
	async alarm(): Promise<void> {
		const now = Date.now();
		const limit = Number(this.env.HOST_SILENCE_MS) || HOST_SILENCE_MS;
		let dropped = false;
		for (const ws of this.games().values()) {
			const peer = this.peer(ws);
			const pingedAt = this.ctx.getWebSocketAutoResponseTimestamp(ws)?.getTime() ?? 0;
			if (now - Math.max(peer.heardAt ?? 0, pingedAt) > limit) {
				log("host_went_quiet", { game: shortId(peer.gameId), silentSeconds: Math.round((now - Math.max(peer.heardAt ?? 0, pingedAt)) / 1000) });
				this.forget(ws, peer);
				dropped = true;
				try {
					ws.close(1000, "The host went quiet");
				} catch {
					// Already gone.
				}
			}
		}
		if (dropped) {
			this.broadcast();
		}
		await this.scheduleSweep();
	}

	private async create(ws: WebSocket, peer: Peer, message: Message): Promise<void> {
		const ipv4Port = port(message.ipv4Port);
		const ipv6Port = port(message.ipv6Port) || ipv4Port;
		if (!ipv4Port && !ipv6Port) {
			return this.refuse(ws, "No port to reach the host on");
		}
		const replaced = this.takeOver(ws, message);
		if (!peer.game && this.games().size >= MAX_GAMES) {
			return this.refuse(ws, "The lobby is full, try again later");
		}
		const reachable = addresses(peer, message.ipv4, message.ipv6);
		// A host without an IPv6 socket says so with no IPv6 port; never offer an
		// IPv6 address for it, even one its connection came from.
		if (!port(message.ipv6Port)) {
			reachable.ipv6 = "";
		}
		const route: Route = { ...reachable, ipv4Port, ipv6Port, directIpv4Port: port(message.directIpv4Port) };
		const game = newGame();
		readGame(message, game);
		if (replaced) {
			// Keeps its place in the list.
			game.createdAt = replaced.game.createdAt;
		}
		const gameId = crypto.randomUUID().replaceAll("-", "");
		const key = crypto.randomUUID().replaceAll("-", "");
		ws.serializeAttachment({ ip: peer.ip, gameId, game, route, key, heardAt: Date.now() } satisfies Peer);
		log("game_listed", { game: shortId(gameId), replaced: shortId(replaced?.gameId), version: logVersion(game.version), games: this.games().size });
		this.send(ws, JSON.stringify({ type: "created", id: gameId, key }));
		this.broadcast();
		await this.scheduleSweep();
	}

	/**
	 * A host that lost its connection lists its game again on a new one, naming
	 * the old listing and the key it was given for it. The old connection can
	 * still look open from here (the goodbye never arrived): its listing goes now,
	 * not after the silence limit, so the game is never listed twice.
	 */
	private takeOver(ws: WebSocket, message: Message): { gameId: string; game: Game } | undefined {
		if (typeof message.replaces !== "string" || typeof message.key !== "string") {
			return undefined;
		}
		const oldWs = this.games().get(message.replaces);
		if (!oldWs || oldWs === ws) {
			return undefined;
		}
		const old = this.peer(oldWs);
		if (!old.key || !sameKey(old.key, message.key)) {
			return undefined;
		}
		this.forget(oldWs, old);
		try {
			oldWs.close(1000, "Listed again");
		} catch {
			// Already gone.
		}
		return { gameId: old.gameId!, game: old.game! };
	}

	private update(ws: WebSocket, peer: Peer, message: Message): void {
		if (!peer.game || message.id !== peer.gameId) {
			return this.refuse(ws, "That game is not yours");
		}
		readGame(message, peer.game);
		if (message.action === "game_started") {
			peer.game.phase = "In-Game";
			peer.game.joinable = false;
		}
		ws.serializeAttachment(peer);
		if (message.action === "game_started") {
			this.send(ws, JSON.stringify({ type: "game_started", success: true }));
		}
		this.broadcast();
	}

	private close(ws: WebSocket, peer: Peer, message: Message): void {
		if (!peer.game || message.id !== peer.gameId) {
			return this.send(ws, JSON.stringify({ type: "deleted", success: false }));
		}
		log("game_closed", { game: shortId(peer.gameId), by: message.action, phase: peer.game.phase });
		this.forget(ws, peer);
		this.send(ws, JSON.stringify({ type: "deleted", success: true }));
		this.broadcast();
	}

	private punch(ws: WebSocket, message: Message): void {
		const ipv4Port = port(message.myIpv4Port);
		const ipv6Port = port(message.myIpv6Port);
		if (!ipv4Port && !ipv6Port) {
			return this.refuse(ws, "No port to reach you on");
		}
		const role = message.role ?? "player";
		if (role !== "player" && role !== "spectator") {
			return this.refuse(ws, "Unknown role");
		}
		const hostWs = typeof message.lobbyId === "string" ? this.games().get(message.lobbyId) : undefined;
		if (!hostWs) {
			return this.refuse(ws, "That game has closed");
		}
		if (hostWs === ws) {
			return this.refuse(ws, "That is your own game");
		}
		const host = this.peer(hostWs);
		const game = host.game!;
		const route = host.route!;
		const version = firstWord(message.version);
		const hostVersion = firstWord(game.version);
		if (version && hostVersion && version !== hostVersion) {
			return this.refuse(ws, "The host is using a different game version.", Refused.Version);
		}
		const refusal = role === "spectator" ? spectatorRefusal(game, version, hostVersion) : playerRefusal(game);
		if (refusal) {
			return this.refuse(ws, ...refusal);
		}
		const joiner = addresses(this.peer(ws), message.myIpv4, message.myIpv6);
		try {
			hostWs.send(
				JSON.stringify({
					type: "punch",
					peerIpv4: ipv4Port ? joiner.ipv4 : "",
					peerIpv6: ipv6Port ? joiner.ipv6 : "",
					peerIpv4Port: ipv4Port,
					peerIpv6Port: ipv6Port,
				}),
			);
		} catch {
			log("host_unreachable", { game: shortId(host.gameId) });
			this.forget(hostWs, host);
			this.broadcast();
			return this.refuse(ws, "That game has closed");
		}
		const reply: Record<string, unknown> = {
			type: "punch",
			peerIpv4: route.ipv4,
			peerIpv6: route.ipv6,
			peerIpv4Port: route.ipv4Port,
			peerIpv6Port: route.ipv6Port,
		};
		if (route.directIpv4Port) {
			reply.peerDirectIpv4Port = route.directIpv4Port;
		}
		this.send(ws, JSON.stringify(reply));
	}

	private peer(ws: WebSocket): Peer {
		return (ws.deserializeAttachment() as Peer | null) ?? { ip: "" };
	}

	/** Open games by id, each with its host's connection. */
	private games(): Map<string, WebSocket> {
		const games = new Map<string, WebSocket>();
		for (const ws of this.ctx.getWebSockets()) {
			const peer = this.peer(ws);
			if (peer.gameId && peer.game) {
				games.set(peer.gameId, ws);
			}
		}
		return games;
	}

	/** The public list: game details only, never an address or a port. */
	private list(): string {
		const lobbies = [...this.games()]
			.map(([id, ws]) => ({ id, ...this.peer(ws).game! }))
			.sort((a, b) => a.createdAt - b.createdAt);
		return JSON.stringify({ type: "lobbies", lobbies });
	}

	/** Sends the new list to everyone browsing (hosts do not need it). */
	private broadcast(): void {
		const list = this.list();
		for (const ws of this.ctx.getWebSockets()) {
			if (!this.peer(ws).game) {
				this.send(ws, list);
			}
		}
	}

	private forget(ws: WebSocket, peer: Peer): void {
		ws.serializeAttachment({ ip: peer.ip } satisfies Peer);
	}

	private hangUp(ws: WebSocket, why: Record<string, string | number | boolean>): void {
		const peer = this.peer(ws);
		if (peer.game) {
			log("host_disconnected", { game: shortId(peer.gameId), phase: peer.game.phase, listedSeconds: Math.round((Date.now() - peer.game.createdAt) / 1000), ...why });
			this.forget(ws, peer);
			this.broadcast();
		} else {
			log("disconnected", why);
		}
		try {
			ws.close(1000, "Goodbye");
		} catch {
			// Already closed.
		}
	}

	private send(ws: WebSocket, text: string): void {
		try {
			ws.send(text);
		} catch {
			// The connection is closing; its close handler tidies up.
		}
	}

	private refuse(ws: WebSocket, error: string, reason = 0): void {
		this.send(ws, JSON.stringify({ type: "error", error, reason }));
	}

	private async scheduleSweep(): Promise<void> {
		if (this.games().size === 0 || (await this.ctx.storage.getAlarm()) !== null) {
			return;
		}
		const interval = Number(this.env.SWEEP_INTERVAL_MS) || SWEEP_INTERVAL_MS;
		await this.ctx.storage.setAlarm(Date.now() + interval);
	}
}

export default {
	async fetch(request: Request, env: Env): Promise<Response> {
		const { pathname } = new URL(request.url);
		if (pathname === "/ws") {
			return env.LOBBY.get(env.LOBBY.idFromName("lobby")).fetch(request);
		}
		if (pathname === "/ip") {
			// The bare address: the game pastes it into JSON as it is.
			return new Response(request.headers.get("CF-Connecting-IP") ?? "", {
				headers: { "Content-Type": "text/plain; charset=utf-8", "Cache-Control": "no-store" },
			});
		}
		if (pathname === "/") {
			return new Response("KeeperFX: Tux Edition lobby server\n", { headers: { "Content-Type": "text/plain; charset=utf-8" } });
		}
		return new Response("Not found\n", { status: 404 });
	},
} satisfies ExportedHandler<Env>;
