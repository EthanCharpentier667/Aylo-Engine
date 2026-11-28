-- The scene around the game: a space background whose layers scroll at their own speed (a Tiled map), where a planet
-- passes now and then. The camera and the obstacles' look are in the arena's scene (assets/scenes/arena.scene).

local assets = require("@scripts/client/assets")
local Duel = require("@scripts/shared/arena")

local backgroundSize = { x = 272, y = 160 } -- the map's images, in pixels: scaled to the view's height
local backgroundScale = Duel.viewHeight / backgroundSize.y
local backgroundDrawLayer = -100 -- under every sprite; the planets are over its three layers
local firstPlanetDelay = 8 -- seconds before the first planet passes
local planetDelayMin, planetDelayMax = 25, 50 -- seconds between two planets

-- Startup: the background, its parallax origin (the map's centre) where the camera looks.
engine.ecs.system("game.SpawnBackground", {
	phase = "Startup",
	side = "client",
	query = {},
	calls = { "aylo.tilemap.spawn" },
	run = function(ctx)
		ctx.call("aylo.tilemap.spawn", {
			map = assets.background,
			position = {
				x = (Duel.viewWidth - backgroundSize.x * backgroundScale) / 2,
				y = (Duel.viewHeight - backgroundSize.y * backgroundScale) / 2,
			},
			scale = { x = backgroundScale, y = backgroundScale },
			firstDrawLayer = backgroundDrawLayer,
		})
	end,
})

local countdown = firstPlanetDelay -- seconds before the next planet

-- Update: now and then, one of the planets' actors passes; it places itself and goes (game.BackgroundPlanet).
engine.ecs.system("game.SpawnPlanets", {
	phase = "Update",
	side = "client",
	query = {},
	calls = { "aylo.capsule.spawn" },
	run = function(ctx)
		countdown -= ctx.time.delta
		if countdown > 0 then
			return
		end
		countdown = planetDelayMin + math.random() * (planetDelayMax - planetDelayMin)
		engine.capsule.spawn(ctx, Duel.planets[math.random(1, #Duel.planets)])
	end,
})
