-- The look of what the server spawns: the ships and the lasers fly animated where the server puts them, and the loser
-- explodes. The server spawns them from their actors on its side only, so their look is given here, on the clients.

local assets = require("@scripts/client/assets")
local Duel = require("@scripts/shared/arena")

-- PostUpdate: gives the ships their sprite, animated. PostUpdate also runs while the editor edits, so a ship's actor
-- opened alone shows its look.
engine.ecs.system("game.DressShips", {
	phase = "PostUpdate",
	side = "client",
	query = { read = { "game.Ship" }, without = { "aylo.SpriteRenderer" } },
	run = function(ctx)
		for entity, ship in ctx.query do
			ctx.commands:add(entity, "aylo.Transform2D", { position = ship.position })
			ctx.commands:add(entity, "aylo.SpriteRenderer", { layer = 1 })
			ctx.commands:add(entity, "aylo.SpriteAnimator", {
				atlas = if ship.player == 0 then assets.leftShip else assets.rightShip,
				animation = "fly",
			})
		end
	end,
})

-- PostUpdate: gives the lasers their sprite, flying away from their shooter (the editor shows it too).
engine.ecs.system("game.DressLasers", {
	phase = "PostUpdate",
	side = "client",
	query = { read = { "game.Laser" }, without = { "aylo.SpriteRenderer" } },
	run = function(ctx)
		for entity, laser in ctx.query do
			ctx.commands:add(entity, "aylo.Transform2D", { position = laser.position })
			ctx.commands:add(entity, "aylo.SpriteRenderer", { layer = 2 })
			ctx.commands:add(entity, "aylo.SpriteAnimator", {
				atlas = assets.laser,
				animation = if laser.shooter == 0 then "right" else "left",
			})
		end
	end,
})

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
