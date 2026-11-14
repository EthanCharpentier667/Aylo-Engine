-- The lasers: they fly with the physics; the first thing one touches spends it, and the other player's ship ends the
-- round.

local Duel = require("@scripts/shared/arena")

-- The lasers of this tick, by their entity's bits: their shooter. Filled by FollowLaserBodies for DetectHits.
local lasers: { [number]: number } = {}

-- FixedUpdate, after the physics moved the lasers: replicates where they are and removes those that left the view.
engine.ecs.system("game.FollowLaserBodies", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "aylo.Transform2D" }, write = { "game.Laser" } },
	after = { "aylo.Physics.SyncFromPhysics" },
	run = function(ctx)
		lasers = {}
		for entity, transform, laser in ctx.query do
			laser.position = transform.position
			lasers[entity.bits] = laser.shooter
			local x = transform.position.x
			if x < -Duel.laserHalfSize.x or x > Duel.viewWidth + Duel.laserHalfSize.x then
				ctx.commands:despawn(entity)
			end
		end
	end,
})

-- FixedUpdate, after the physics sent its events: a laser that touches something is spent; one that touches a ship
-- (always the other player's, by the collision layers) wins the round, if it is still Live.
engine.ecs.system("game.DetectHits", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Ship" } },
	events = { read = { "aylo.CollisionStarted" } },
	calls = { "aylo.state.get", "aylo.state.set" },
	after = { "aylo.Physics.Events", "game.FollowLaserBodies" },
	run = function(ctx)
		local ships = {}
		for entity in ctx.query do
			ships[entity.bits] = true
		end
		local live = ctx.call("aylo.state.get", { state = "game.Round" }) == "Live"
		local spent = {}
		for collision in ctx.events.read("aylo.CollisionStarted") do
			for _, pair in { { collision.first, collision.second }, { collision.second, collision.first } } do
				local laser, other = pair[1], pair[2]
				local shooter = lasers[laser.bits]
				if shooter == nil or spent[laser.bits] then
					continue
				end
				spent[laser.bits] = true
				ctx.commands:despawn(laser)
				if live and ships[other.bits] then
					live = false -- no other hit wins this round
					ctx.commands:spawn({ ["game.Winner"] = { player = shooter } })
					ctx.call("aylo.state.set", { state = "game.Round", value = "Over" })
					print(`[server] player {shooter + 1} wins the round`)
				end
			end
		end
	end,
})
