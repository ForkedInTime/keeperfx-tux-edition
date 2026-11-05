-- serialization.lua
-- Internal serialization logic for Lua<->C data.
-- Used by the engine to serialize complex Lua state; not intended for API users.

-- This edition: what this module uses from the debug library, and load and string.dump, are kept
-- here because scripts loaded after the game's own library (campaigns, maps, mods) only get
-- debug.traceback and debug.getinfo; see src/lua_sandbox.c.
local debug = debug
local load = load
local dump = string.dump

local binser = require 'external.binser'
require 'classes.Pos3d'
local serialization_version = 4
local legacy_serialization_version = 3 -- functions as bytecode; read, never written

local registry = debug.getregistry()
for _, name in ipairs({ "Player", "Thing", "Slab", "Camera", "Room", "Map", "Pos3d" }) do
    binser.registerResource(registry[name], "metatable:" .. name)
end

-- Registers every function (Lua or C) reachable from source by a stable string-keyed
-- path as a binser resource, so it serializes as a name instead of bytecode.
local registered_functions = {}
local scanned_tables = {}
local function register_named_functions(source, prefix)
    if scanned_tables[source] then
        return
    end
    scanned_tables[source] = true
    local names = {}
    for name in pairs(source) do
        if type(name) == "string" then
            names[#names + 1] = name
        end
    end
    table.sort(names)
    for _, name in ipairs(names) do
        local value = rawget(source, name)
        local path = prefix .. name
        if type(value) == "function" then
            if not registered_functions[value] then
                binser.registerResource(value, path)
                registered_functions[value] = true
            end
        elseif type(value) == "table" then
            register_named_functions(value, path .. ".")
        end
    end
end
register_named_functions(_G, "global:")
register_named_functions(registry, "registry:")

-- FNV-1a, just to detect "this script text changed since the save" -- not a security hash.
local function source_hash(text)
    local bit = _G.bit
    local h = 2166136261
    for i = 1, #text do
        h = bit.bxor(h, text:byte(i))
        h = bit.band(h * 16777619, 0xFFFFFFFF)
    end
    return h
end

local function read_source_lines(file, first, last)
    local f = io.open(file, "r")
    if not f then
        return nil
    end
    local lines = {}
    local n = 0
    for line in f:lines() do
        n = n + 1
        if n >= first and n <= last then
            lines[#lines + 1] = line
        end
        if n > last then
            break
        end
    end
    f:close()
    if n < last then
        return nil -- file is shorter than the save expects
    end
    return table.concat(lines, "\n")
end

--------------------------------------------------------------------------------------------------
-- This edition: functions are saved as their source text, never as bytecode.
--
-- LuaJIT runs bytecode without checking it, and the save, a multiplayer resync and the Game table
-- itself are all data that a map or another player can shape, so bytecode read back from any of
-- them could reach the machine (see src/lua_sandbox.c). A function is written as the exact text
-- that defines it: found by trying the function...end spans on its first and last line and keeping
-- the one that compiles to the same code as the live function, with its chunk name, its first line
-- and those upvalues that are plain values. Reading it back compiles that text, in text mode only,
-- which can do nothing a map's own script cannot. Unlike reading the script again at load time,
-- this restores the function as it was saved, so a later edit to the script does not break the
-- save. A function whose text cannot be found comes back as a stub that logs when it is called.
--------------------------------------------------------------------------------------------------

local MAX_LINE = 1000000

-- How each function is written, found once: fn -> { src, chunk, line, names } or { stub, where, reason }
local definitions = setmetatable({}, { __mode = "k" })

local function is_plain(v)
    local t = type(v)
    return t == "number" or t == "string" or t == "boolean"
end

local function upvalue_names(fn)
    local names = {}
    local i = 1
    while true do
        local name = debug.getupvalue(fn, i)
        if not name then
            return names
        end
        names[i] = name
        i = i + 1
    end
end

local function valid_names(names)
    for i = 1, #names do
        if type(names[i]) ~= "string" or not names[i]:match("^[%a_][%w_]*$") then
            return false
        end
    end
    return true
end

-- The ULEB128 number at s:byte(i) and the index after it, or nil past the end of s
local function read_uleb(s, i)
    local v, mul = 0, 1
    while true do
        local b = s:byte(i)
        if not b then
            return nil
        end
        i = i + 1
        v = v + (b % 128) * mul
        if b < 128 then
            return v, i
        end
        mul = mul * 128
    end
end

-- A dump's prototypes without what differs between a function and a copy compiled from the same
-- text: the dump's header and debug info, the top function's upvalue descriptors (they name a
-- local of the enclosing function, which a copy compiled with stand-in locals has elsewhere), and
-- the flag bits LuaJIT changes at run time (the top ones count the closures made from the
-- prototype: seven triggers registered from one line of Events.lua leave it at 4).
-- Takes stripped and full dumps alike, so a full dump from an old save compares with a stripped
-- dump of a candidate. Reads bytes only: a dump given here is never loaded.
local function normalized_code(s)
    if type(s) ~= "string" or s:sub(1, 3) ~= "\27LJ" then
        return nil
    end
    local flags, i = read_uleb(s, 5)
    if not flags then
        return nil
    end
    local stripped = flags % 4 >= 2
    if not stripped then
        local namelen
        namelen, i = read_uleb(s, i)
        if not namelen then
            return nil
        end
        i = i + namelen
    end
    local protos = {}
    while true do
        local len, j = read_uleb(s, i)
        if not len then
            return nil
        end
        if len == 0 then
            break
        end
        local stop = j + len -- the prototypes are written inner first, the top function last
        local hdr = s:sub(j, j + 3) -- flags, parameters, frame size, upvalues
        if stop - 1 > #s or #hdr < 4 then
            return nil
        end
        local kgc, kn, nbc, q
        kgc, q = read_uleb(s, j + 4)
        if q then kn, q = read_uleb(s, q) end
        if q then nbc, q = read_uleb(s, q) end
        local dbg = 0
        if q and not stripped then
            dbg, q = read_uleb(s, q)
            if q and dbg > 0 then
                local _
                _, q = read_uleb(s, q) -- first line
                if q then _, q = read_uleb(s, q) end -- line count
            end
        end
        if not q or stop - dbg < q then
            return nil
        end
        -- flags: keep inner functions, vararg and FFI constants (lj_obj.h PROTO_CHILD|VARARG|FFI)
        hdr = string.char(hdr:byte(1) % 8) .. hdr:sub(2)
        protos[#protos + 1] = { head = hdr .. kgc .. "," .. kn .. "," .. nbc, body = s:sub(q, stop - dbg - 1),
                                nbc = nbc, nuv = hdr:byte(4) }
        i = stop
    end
    local top = protos[#protos]
    if not top then
        return nil
    end
    local uv = top.nbc * 4 -- the upvalue descriptors follow the instructions
    top.body = top.body:sub(1, uv) .. string.rep("\0", top.nuv * 2) .. top.body:sub(uv + top.nuv * 2 + 1)
    local out = {}
    for n, p in ipairs(protos) do
        out[n] = p.head .. ";" .. p.body
    end
    return table.concat(out, "|")
end

-- fn's compiled form, comparable with another compile of the same text (normalized_code)
local function code_of(fn)
    local ok, s = pcall(dump, fn, true)
    return ok and normalized_code(s) or nil
end

local function split_lines(text)
    local lines = {}
    for line in (text .. "\n"):gmatch("(.-)\r?\n") do
        lines[#lines + 1] = line
    end
    return lines
end

-- { start, end } of each place word stands on line as a whole word
local function word_positions(line, word)
    local out, from = {}, 1
    while true do
        local s, e = line:find(word, from, true)
        if not s then
            return out
        end
        if not line:sub(s - 1, s - 1):match("[%w_]") and not line:sub(e + 1, e + 1):match("[%w_]") then
            out[#out + 1] = { s, e }
        end
        from = e + 1
    end
end

local function span_text(lines, first, last, s, e)
    if first == last then
        return lines[first]:sub(s, e)
    end
    local parts = { lines[first]:sub(s) }
    for n = first + 1, last - 1 do
        parts[#parts + 1] = lines[n]
    end
    parts[#parts + 1] = lines[last]:sub(1, e)
    return table.concat(parts, "\n")
end

-- The top function's instructions in fn's stripped dump, with its constant counts
local function top_instructions(fn)
    local ok, s = pcall(dump, fn, true)
    if not ok then
        return nil
    end
    local _, i = read_uleb(s, 5)
    local top
    while i do
        local len, j = read_uleb(s, i)
        if not len or len == 0 then
            break
        end
        top, i = j, j + len
    end
    if not top then
        return nil
    end
    local kgc, q = read_uleb(s, top + 4)
    local kn, nbc
    kn, q = read_uleb(s, q)
    nbc, q = read_uleb(s, q)
    return kgc .. "," .. kn .. ";" .. s:sub(q, q + nbc * 4 - 1)
end

-- What a chunk made by compile_function compiles to when its text is one function expression:
-- the stand-in locals, then the closure returned (the same for any function, given the count)
local expression_shapes = {}
local function expression_shape(prelude, count)
    if not expression_shapes[count] then
        expression_shapes[count] = top_instructions(load(prelude .. "return function() end", "=shape", "t"))
    end
    return expression_shapes[count]
end

-- "function f(x)" -> "function(x)", "function a.b:c(x)" -> "function(self, x)": a named definition
-- (local function f, function M.f) as the expression it compiles the same as
local function as_expression(text)
    local name, rest = text:match("^function%s+([%a_][%w_%.:]*)%s*%((.*)$")
    if not name then
        return text
    end
    if name:find(":", 1, true) then
        return "function(self" .. (rest:match("^%s*%)") and "" or ", ") .. rest
    end
    return "function(" .. rest
end

-- Compiles function text as the expression it was, on its own line so that errors and tracebacks
-- point where they always did. Its upvalues are declared as locals right before it, so that it
-- captures them as upvalues again rather than reading globals of the same names. The chunk only
-- runs once it is known to do nothing but return that one function: a saved game's text is not
-- run as code, whatever it says.
local function compile_function(text, names, chunkname, line)
    local prelude = (#names > 0) and ("local " .. table.concat(names, ", ") .. " ") or ""
    local chunk, err = load(string.rep("\n", line - 1) .. prelude .. "return " .. text, chunkname, "t", _G)
    if not chunk then
        return nil, err
    end
    local shape = expression_shape(prelude, #names)
    if not shape or top_instructions(chunk) ~= shape then
        return nil, "the text is not a single function"
    end
    local ok, fn = pcall(chunk)
    if not ok then
        return nil, tostring(fn)
    end
    if type(fn) ~= "function" then
        return nil, "the text is not a function"
    end
    return fn
end

-- Every function...end span on lines first..last, outermost end first
local function each_span(lines, first, last)
    return coroutine.wrap(function()
        local ends = word_positions(lines[last], "end")
        for _, st in ipairs(word_positions(lines[first], "function")) do
            for k = #ends, 1, -1 do
                if first ~= last or ends[k][1] > st[2] then
                    coroutine.yield(as_expression(span_text(lines, first, last, st[1], ends[k][2])))
                end
            end
        end
    end)
end

local function where_of(info)
    return info.short_src .. ":" .. tostring(info.linedefined)
end

-- The source text of the chunk fn comes from, or nil
local function chunk_text(info)
    local src = info.source
    local kind = src:sub(1, 1)
    if kind == "@" then
        local f = io.open(src:sub(2), "r")
        if not f then
            return nil
        end
        local text = f:read("*a")
        f:close()
        return text
    elseif kind == "=" then
        return nil -- a chunk loaded under a name of its own: its text is gone
    end
    return src -- a chunk loaded from a string is named after its own text
end

-- How fn is written, or nil and the reason it cannot be
local function find_definition(fn)
    local info = debug.getinfo(fn, "S")
    if info.what == "C" then
        return nil, "it is a C function"
    end
    local names = upvalue_names(fn)
    if not valid_names(names) then
        return nil, "its upvalues have no names"
    end
    local text = chunk_text(info)
    if not text then
        return nil, "its source text is not available"
    end
    local chunkname = (info.source:sub(1, 1) == "@") and info.source or "=saved function"
    local want = code_of(fn)
    if not want then
        return nil, "its code cannot be read"
    end
    if info.what == "main" then
        local chunk = load(text, chunkname, "t", _G)
        if chunk and code_of(chunk) == want then
            return { src = text, chunk = chunkname, line = 0, names = names }
        end
        return nil, "its script has changed since it was loaded"
    end
    local first, last = info.linedefined, info.lastlinedefined
    local lines = split_lines(text)
    if not lines[first] or not lines[last] then
        return nil, "its script has changed since it was loaded"
    end
    for src in each_span(lines, first, last) do
        local candidate = compile_function(src, names, chunkname, first)
        if candidate and code_of(candidate) == want then
            return { src = src, chunk = chunkname, line = first, names = names }
        end
    end
    return nil, "its text was not found in the script"
end

local function make_stub(where, reason)
    where, reason = tostring(where), tostring(reason)
    local warned = false
    local function stub()
        if not warned then
            warned = true
            print(("Serialisation: the function saved from %s could not be restored (%s), so it did nothing"):format(where, reason))
        end
    end
    definitions[stub] = { stub = true, where = where, reason = reason }
    return stub
end

local encoded = 0 -- functions written by the current GetSerializedData

local function encode_function(fn)
    local def = definitions[fn]
    if def == nil then
        local reason
        def, reason = find_definition(fn)
        if not def then
            local where = where_of(debug.getinfo(fn, "S"))
            print(("Serialisation: the function at %s cannot be saved (%s); after loading, calling it does nothing"):format(where, reason))
            def = { stub = true, where = where, reason = reason }
        end
        definitions[fn] = def
    end
    if def.stub then
        encoded = encoded + 1
        return { stub = true, where = def.where, reason = def.reason }
    end
    local up = {}
    for i, name in ipairs(def.names) do
        local _, value = debug.getupvalue(fn, i)
        -- itself: a local function calling itself by name
        up[i] = { name = name, value = is_plain(value) and value or nil, self = (value == fn) or nil }
    end
    encoded = encoded + 1
    return { src = def.src, chunk = def.chunk, line = def.line, up = up }
end

local function decode_function(spec)
    if type(spec) ~= "table" then
        return make_stub("?", "the save holds no function here")
    end
    if spec.stub then
        return make_stub(spec.where, spec.reason)
    end
    local src, chunk, line, up = spec.src, spec.chunk, spec.line, spec.up
    if type(src) ~= "string" or type(chunk) ~= "string" or type(up) ~= "table" or type(line) ~= "number"
        or line < 0 or line > MAX_LINE or line ~= math.floor(line) then
        return make_stub("?", "the saved function is malformed")
    end
    local where = chunk:gsub("^[@=]", "") .. ":" .. line
    local names = {}
    for i, uv in ipairs(up) do
        names[i] = type(uv) == "table" and uv.name or nil
    end
    if #names ~= #up or not valid_names(names) then
        return make_stub(where, "the saved function is malformed")
    end
    local fn, err
    if line == 0 then
        fn, err = load(src, chunk, "t", _G)
    else
        fn, err = compile_function(src, names, chunk, line)
    end
    if not fn then
        return make_stub(where, err)
    end
    for i, uv in ipairs(up) do
        if debug.getupvalue(fn, i) == uv.name then
            if uv.self == true then
                debug.setupvalue(fn, i, fn)
            elseif is_plain(uv.value) then
                debug.setupvalue(fn, i, uv.value)
            end
        end
    end
    definitions[fn] = { src = src, chunk = chunk, line = line, names = names }
    return fn
end

--------------------------------------------------------------------------------------------------
-- Saves from before this edition stopped writing bytecode (serialisation version 3, up to
-- 1.4.0.6062) hold a function either as { __anon_fn = true, file, linedefined, lastlinedefined,
-- hash, upvalues, bytecode } or as a binser FUNCTION record of raw bytecode. Both are rebuilt from
-- the script where it still reads the same at those lines. The bytecode itself is never loaded.
--------------------------------------------------------------------------------------------------

local legacy = { rebuilt = 0, stubbed = 0 }

local function legacy_stub(where, reason)
    legacy.stubbed = legacy.stubbed + 1
    return make_stub(where, reason)
end

-- The function on lines first..last of file, compiled with names as its upvalues, whose shape
-- (lines, upvalue count and, when known, parameter count) is the saved one. When several are, the
-- one whose code equals the saved bytecode's (compared as bytes, never loaded); with no such
-- match (bytecode from another LuaJIT build, say) only a single candidate is taken. nil otherwise.
local function rebuild_from_script(file, first, last, names, nparams, hash, saved_bytecode)
    local text = read_source_lines(file, first, last)
    if not text or (hash ~= nil and source_hash(text) ~= hash) then
        return nil
    end
    local lines = {}
    for i, line in ipairs(split_lines(text)) do
        lines[first + i - 1] = line
    end
    if not lines[first] or not lines[last] then
        return nil
    end
    local chunkname = "@" .. file
    local matches = {}
    for src in each_span(lines, first, last) do
        local fn = compile_function(src, names, chunkname, first)
        if fn then
            local info = debug.getinfo(fn, "Su")
            if info.linedefined == first and info.lastlinedefined == last and info.nups == #names
                and (nparams == nil or info.nparams == nparams) then
                matches[#matches + 1] = { fn = fn, src = src }
            end
        end
    end
    local ok, saved = pcall(normalized_code, saved_bytecode)
    local pick
    if ok and saved then
        for _, m in ipairs(matches) do
            if code_of(m.fn) == saved then
                pick = m
                break
            end
        end
    end
    if not pick and #matches == 1 then
        pick = matches[1]
    end
    if not pick then
        return nil
    end
    definitions[pick.fn] = { src = pick.src, chunk = chunkname, line = first, names = names }
    return pick.fn
end

local function decode_legacy_spec(spec)
    local file, first, last, upvalues = spec.file, spec.linedefined, spec.lastlinedefined, spec.upvalues
    local where = tostring(file) .. ":" .. tostring(first)
    if type(file) ~= "string" or type(first) ~= "number" or type(last) ~= "number" or type(upvalues) ~= "table"
        or first < 1 or last < first or last > MAX_LINE then
        return legacy_stub(where, "the saved function is malformed")
    end
    local names = {}
    for i, uv in ipairs(upvalues) do
        names[i] = type(uv) == "table" and uv.name or nil
    end
    if #names ~= #upvalues or not valid_names(names) then
        return legacy_stub(where, "the saved function is malformed")
    end
    local fn = rebuild_from_script(file, first, last, names, nil, spec.hash, spec.bytecode)
    if not fn then
        return legacy_stub(where, "saved by an earlier version, and its script has changed since")
    end
    for i, uv in ipairs(upvalues) do
        if is_plain(uv.value) and debug.getupvalue(fn, i) == uv.name then
            debug.setupvalue(fn, i, uv.value)
        end
    end
    legacy.rebuilt = legacy.rebuilt + 1
    return fn
end

-- A binser FUNCTION record: LuaJIT bytecode, read here as bytes for the script and lines it came
-- from, its parameter count and its upvalue names. Never loaded.
local function read_bytecode_origin(bc)
    if bc:sub(1, 3) ~= "\27LJ" then
        return nil
    end
    local flags, i = read_uleb(bc, 5)
    if not flags or flags % 4 >= 2 then
        return nil -- stripped: no chunk name, no lines
    end
    local namelen
    namelen, i = read_uleb(bc, i)
    local chunkname = bc:sub(i, i + namelen - 1)
    i = i + namelen
    local top, toplen
    while true do
        local len, j = read_uleb(bc, i)
        if not len or len == 0 then
            break
        end
        top, toplen, i = j, len, j + len
    end
    if not top or top + toplen - 1 > #bc then
        return nil
    end
    local nparams, nuv = bc:byte(top + 1), bc:byte(top + 3)
    local _, q = read_uleb(bc, top + 4) -- object constants
    _, q = read_uleb(bc, q) -- number constants
    local nbc, sizedbg, firstline, numline
    nbc, q = read_uleb(bc, q)
    sizedbg, q = read_uleb(bc, q)
    if not sizedbg or sizedbg == 0 then
        return nil
    end
    firstline, q = read_uleb(bc, q)
    numline, q = read_uleb(bc, q)
    -- The debug info closes the prototype: a line per instruction, then the upvalue names
    local p = top + toplen - sizedbg + nbc * ((numline < 256) and 1 or ((numline < 65536) and 2 or 4))
    local names = {}
    for n = 1, nuv do
        local z = bc:find("\0", p, true)
        if not z or z > top + toplen - 1 then
            return nil
        end
        names[n] = bc:sub(p, z - 1)
        p = z + 1
    end
    return chunkname, firstline, firstline + numline, nparams, names
end

local function decode_legacy_bytecode(bc)
    local ok, chunkname, first, last, nparams, names = pcall(read_bytecode_origin, bc)
    if not ok or not chunkname then
        return legacy_stub("?", "saved by an earlier version without its source location")
    end
    local where = chunkname:gsub("^[@=]", "") .. ":" .. tostring(first)
    if chunkname:sub(1, 1) ~= "@" or first < 1 or last < first or last > MAX_LINE or not valid_names(names) then
        return legacy_stub(where, "saved by an earlier version, not from a script file")
    end
    local fn = rebuild_from_script(chunkname:sub(2), first, last, names, nparams, nil, bc)
    if not fn then
        return legacy_stub(where, "saved by an earlier version, and its script has changed since")
    end
    legacy.rebuilt = legacy.rebuilt + 1
    return fn -- with its upvalues nil, as such functions always came back
end

binser.setFunctionCodec(encode_function, decode_function, decode_legacy_bytecode)

local function restore_anon_functions(value, seen)
    if type(value) ~= "table" or seen[value] then
        return
    end
    seen[value] = true
    for k, v in pairs(value) do
        if type(v) == "table" then
            -- rawget, not v.__anon_fn: v may be a Thing/Player/etc proxy whose metatable
            -- has its own __index, which a plain index would needlessly go through.
            if rawget(v, "__anon_fn") then
                value[k] = decode_legacy_spec(v)
            else
                restore_anon_functions(v, seen)
            end
        end
    end
end

function GetSerializedData()
    assert(type(Game) == "table", "Game state must be a table")
    encoded = 0
    local ok, result = pcall(binser.serialize, Game, serialization_version)
    if ok and encoded == 0 then
        -- No function in it: the same data in version 3, which 1.4.0.6062 and earlier can read too
        -- (the engine updates keep as the previous one); version 3 only ever differed in functions
        ok, result = pcall(binser.serialize, Game, legacy_serialization_version)
    end
    if not ok then
        error("binser failed: " .. tostring(result))
    end
    return result
end

function SetSerializedData(serialized_data)
    legacy.rebuilt, legacy.stubbed = 0, 0
    local values, count = binser.deserialize(serialized_data)
    local restored = values[1]
    assert(type(restored) == "table", "Serialized game state must be a table")
    local version = values[2]
    assert(count == 2 and (version == serialization_version or version == legacy_serialization_version),
        "Save was made with an incompatible Lua serialization version (" .. tostring(version) .. ")")
    if version == legacy_serialization_version then
        restore_anon_functions(restored, {})
    end
    if legacy.rebuilt + legacy.stubbed > 0 then
        print(("Serialisation: %d function(s) from an earlier version's save rebuilt from their scripts, %d could not be"):format(legacy.rebuilt, legacy.stubbed))
    end
    Game = restored
end

-- Deepcopy tables, booleans, numbers and strings only.
local function plain_copy(value, copies)
    local kind = type(value)
    if kind == "nil" or kind == "boolean" or kind == "number" or kind == "string" then
        return value
    end
    if kind ~= "table" then
        error("can't store a " .. kind, 0)
    end
    if copies[value] then
        return copies[value]
    end
    local copy = {}
    copies[value] = copy
    for k, v in next, value do
        local key_kind = type(k)
        if key_kind ~= "boolean" and key_kind ~= "number" and key_kind ~= "string" then
            error("can't store a " .. key_kind .. " key", 0)
        end
        copy[k] = plain_copy(v, copies)
    end
    return copy
end

function SerializeCustomIntralevelData(value)
    return binser.serialize(plain_copy(value, {}))
end

-- The blob comes from a save, a campaign progress file or a resync, which anyone
-- can edit. binser's function and resource records apply to every deserialize, so
-- copy the result through plain_copy as well: only plain data comes back, and a
-- function or a named resource in a crafted blob is an error (lua_api.c's pcall
-- turns it into nil) instead of a live value handed to the campaign's script.
function DeserializeCustomIntralevelData(serialized_data)
    local values, count = binser.deserialize(serialized_data)
    assert(count == 1, "Custom intralevel data must hold one value")
    return plain_copy(values[1], {})
end
