-- The players: a client asks for a seat from its main menu (game.Join), and each of the first two gets a ship, removed
-- when they go back to the menu (game.Leave) or leave the game.

local Duel = require("@scripts/shared/arena")

-- Only with the network: players join through it.
if not Duel.networked then
	return
end

print(`[server] waiting for {Duel.playerCount} players`)

-- Update: gives a free seat to each client that asks, a ship (assets/actors/ship_left.actor or ship_right.actor),
-- kinematic: MoveShips moves it, the lasers hit it, nothing pushes it. Every request is answered, with the seat or with
-- a refusal when both are taken; a client that asks again gets its own seat back.
engine.ecs.system("game.SeatPlayers", {
	phase = "Update",
	side = "server",
	query = { read = { "game.Ship", "net.Owner" } },
	events = { read = { "net.PacketReceived" } },
	calls = { "aylo.capsule.spawn", "aylo.net.sendToClients" },
	run = function(ctx)
		local seatOf: { [number]: number } = {} -- client -> player
		local taken: { [number]: boolean } = {} -- player -> whether someone has it
		for _, ship, owner in ctx.query do
			seatOf[owner.client] = ship.player
			taken[ship.player] = true
		end
		for _, packet in engine.net.received(ctx) do
			if packet.name ~= "game.Join" then
				continue
			end
			local client = packet.sender
			local player = seatOf[client]
			if player == nil then
				player = if not taken[0] then 0 elseif not taken[1] then 1 else nil
			end
			if player ~= nil and seatOf[client] == nil then
				seatOf[client] = player
				taken[player] = true
				-- The ship's actor holds its place, its body and its collision layers; the client steers it.
				local ship = engine.capsule.spawn(ctx, Duel.ships[player])
				ctx.commands:add(ship, "net.Owner", { client = client })
				ctx.commands:add(ship, "net.PlayerInput", {})
				print(`[server] client {client} is player {player + 1}`)
			elseif player == nil then
				print(`[server] client {client} asked for a seat, but the game is full`)
			end
			engine.net.sendToClients(ctx, "game.Seat", { granted = player ~= nil, player = player or 0 }, client)
		end
	end,
})

-- Update: removes the ship of a client who went back to the menu or left.
engine.ecs.system("game.RemovePlayers", {
	phase = "Update",
	side = "server",
	query = { read = { "net.Owner" }, with = { "game.Ship" } },
	events = { read = { "net.ClientDisconnected", "net.PacketReceived" } },
	run = function(ctx)
		local left = {}
		for client in ctx.events.read("net.ClientDisconnected") do
			left[client.client] = true
		end
		for _, packet in engine.net.received(ctx) do
			if packet.name == "game.Leave" then
				left[packet.sender] = true
			end
		end
		if next(left) == nil then
			return
		end
		for entity, owner in ctx.query do
			if left[owner.client] then
				ctx.commands:despawn(entity)
				print(`[server] client {owner.client} left their seat`)
			end
		end
	end,
})
