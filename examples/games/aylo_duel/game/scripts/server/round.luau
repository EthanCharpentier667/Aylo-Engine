-- The round (game.Round, declared in shared/rules.luau): the server walks it, and the clients follow.
--
--   Waiting --both seats taken--> Live --a laser hits a ship--> Over --Duel.rematchDelay seconds--> Waiting
--                                   \--a player leaves--> Waiting
--
-- Each round starts clean: on entering Live, the ships go back to their start and the shots in flight vanish.

local Duel = require("@scripts/shared/arena")

-- Only with the network: run alone, there are no players to wait for.
if not Duel.networked then
	return
end

-- Update: starts the round once both seats are taken, and stops it when a player leaves.
engine.ecs.system("game.WatchSeats", {
	phase = "Update",
	side = "server",
	query = { with = { "game.Ship" } },
	calls = { "aylo.state.get", "aylo.state.set" },
	after = { "game.SeatPlayers", "game.RemovePlayers" },
	run = function(ctx)
		local ships = 0
		for _ in ctx.query do
			ships += 1
		end
		local round = ctx.call("aylo.state.get", { state = "game.Round" })
		if round == "Waiting" and ships == Duel.playerCount then
			ctx.call("aylo.state.set", { state = "game.Round", value = "Live" })
			print("[server] the round starts")
		elseif round == "Live" and ships < Duel.playerCount then
			ctx.call("aylo.state.set", { state = "game.Round", value = "Waiting" })
			print("[server] a player left: waiting for another")
		end
	end,
})

-- On entering Live: each ship is spawned again from its actor, at its start, for the same client.
engine.ecs.system("game.ResetShips", {
	onEnter = { "game.Round", "Live" },
	side = "server",
	query = { read = { "game.Ship", "net.Owner" } },
	calls = { "aylo.capsule.spawn" },
	run = function(ctx)
		for entity, ship, owner in ctx.query do
			ctx.commands:despawn(entity)
			local fresh = engine.capsule.spawn(ctx, Duel.ships[ship.player])
			ctx.commands:add(fresh, "net.Owner", { client = owner.client })
			ctx.commands:add(fresh, "net.PlayerInput", {})
		end
	end,
})

-- On entering Live: the lasers of the last round vanish.
engine.ecs.system("game.ClearLasers", {
	onEnter = { "game.Round", "Live" },
	side = "server",
	query = { with = { "game.Laser" } },
	after = { "game.ResetShips" },
	run = function(ctx)
		for entity in ctx.query do
			ctx.commands:despawn(entity)
		end
	end,
})

-- On leaving Over: the last winner is forgotten.
engine.ecs.system("game.ClearWinner", {
	onExit = { "game.Round", "Over" },
	side = "server",
	query = { with = { "game.Winner" } },
	run = function(ctx)
		for entity in ctx.query do
			ctx.commands:despawn(entity)
		end
	end,
})

-- Seconds before the next round, counted while the round is Over.
local nextRoundIn = 0

-- On entering Over: the clock of the next round starts.
engine.ecs.system("game.StartRematchClock", {
	onEnter = { "game.Round", "Over" },
	side = "server",
	run = function()
		nextRoundIn = Duel.rematchDelay
	end,
})

-- FixedUpdate, while Over: once the winner was shown long enough, waits for the players of the next round.
engine.ecs.system("game.Rematch", {
	phase = "FixedUpdate",
	side = "server",
	inState = { "game.Round", "Over" },
	calls = { "aylo.state.set" },
	after = { "game.DetectHits" },
	run = function(ctx)
		nextRoundIn -= ctx.time.fixedDelta
		if nextRoundIn <= 0 then
			ctx.call("aylo.state.set", { state = "game.Round", value = "Waiting" })
		end
	end,
})
