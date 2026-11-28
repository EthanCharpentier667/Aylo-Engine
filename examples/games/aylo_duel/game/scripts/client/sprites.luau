-- The look of what the server spawns follows it: the ships, the lasers and the misfires are drawn and sound from their
-- actors (assets/actors/), which every client spawns with them, where the server says they are. The loser explodes.

local Duel = require("@scripts/shared/arena")

-- Update: the ships' sprites follow the positions the server sends.
engine.ecs.system("game.FollowShips", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Ship" }, write = { "aylo.Transform2D" } },
	run = function(ctx)
		for _, ship, transform in ctx.query do
			transform.position = ship.position
		end
	end,
})

-- Update: so do the lasers'.
engine.ecs.system("game.FollowLasers", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Laser" }, write = { "aylo.Transform2D" } },
	after = { "game.FollowShips" },
	before = { "game.BackgroundPlanet.onUpdate" }, -- a Behaviour declares no order: the systems order themselves on it
	run = function(ctx)
		for _, laser, transform in ctx.query do
			transform.position = laser.position
		end
	end,
})

-- Update: and the misfires', so that their sound is heard where they were fired.
engine.ecs.system("game.FollowMisfires", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Misfire" }, write = { "aylo.Transform2D" } },
	after = { "game.FollowLasers" },
	before = { "game.BackgroundPlanet.onUpdate" },
	run = function(ctx)
		for _, misfire, transform in ctx.query do
			transform.position = misfire.position
		end
	end,
})

-- The player who just won, read by game.ExplodeLoser; nil while nobody did.
local newWinner: number? = nil

-- Update: notices the Winner the server spawns, for game.ExplodeLoser.
engine.ecs.system("game.NoticeWinner", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Winner" }, added = { "game.Winner" } },
	run = function(ctx)
		for _, winner in ctx.query do
			newWinner = winner.player
		end
	end,
})

-- Update: once someone won, the loser's ship vanishes in an explosion.
engine.ecs.system("game.ExplodeLoser", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Ship" }, write = { "aylo.SpriteRenderer" } },
	calls = { "aylo.capsule.spawn" },
	after = { "game.NoticeWinner" },
	run = function(ctx)
		if newWinner == nil then
			return
		end
		for _, ship, sprite in ctx.query do
			if ship.player ~= newWinner then
				local color = sprite.color
				sprite.color = { x = color.x, y = color.y, z = color.z, w = 0 } -- its alpha: the ship is gone
				-- The explosion's actor plays once and goes (game.Explosion).
				engine.capsule.spawn(ctx, Duel.explosion, { ["aylo.Transform2D"] = { position = ship.position } })
			end
		end
		newWinner = nil
	end,
})
