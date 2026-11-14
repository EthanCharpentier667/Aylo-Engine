-- The scene around the game: the camera, the obstacles' look, and a space background whose layers scroll at their own
-- speed (a Tiled map), where a planet passes now and then.

local assets = require("@scripts/client/assets")
local Duel = require("@scripts/shared/arena")

local backgroundSize = { x = 272, y = 160 } -- the map's images, in pixels: scaled to the view's height
local backgroundScale = Duel.viewHeight / backgroundSize.y
local backgroundDrawLayer = -100 -- under every sprite
local planetDrawLayer = backgroundDrawLayer + 3 -- over the background's three layers
local firstPlanetDelay = 8 -- seconds before the first planet passes
local planetDelayMin, planetDelayMax = 25, 50 -- seconds between two planets
local planetScaleMin = 0.6 -- the size of the farthest planet, from 1 for the closest...
local planetSpeedMin, planetSpeedMax = 22, 34 -- ... and its speed, in background pixels per second

engine.ecs.component("game.BackgroundPlanet", {
	fields = { width = "f32" }, -- on screen, in world units
	presence = "clientOnly",
})

-- Startup: the camera looks at the middle of the arena and listens there.
engine.ecs.system("game.SpawnCamera", {
	phase = "Startup",
	side = "client",
	query = {},
	run = function(ctx)
		ctx.commands:spawn({
			["aylo.Transform"] = { position = { x = Duel.viewWidth / 2, y = Duel.viewHeight / 2 } },
			["aylo.Camera"] = {},
			["aylo.AudioListener"] = {},
		})
	end,
})

-- PostUpdate: draws the obstacles as their colliders, placed where the server says when they came from it. PostUpdate
-- also runs while the editor edits the arena, so the obstacles show there too.
engine.ecs.system("game.PlaceObstacles", {
	phase = "PostUpdate",
	side = "client",
	query = {
		read = { "game.Obstacle", "aylo.Collider" },
		optionalRead = { "aylo.Transform" },
		without = { "aylo.ShapeRenderer" },
	},
	run = function(ctx)
		for entity, obstacle, collider, transform in ctx.query do
			if transform == nil then -- replicated: the server sends where it is, not its Transform
				ctx.commands:add(entity, "aylo.Transform", { position = obstacle.position })
			end
			ctx.commands:add(entity, "aylo.ShapeRenderer", {
				kind = if collider.shape == "circle" then "circle" else "rect",
				size = collider.size,
				radius = collider.radius,
				cornerRadius = 6,
				fillColor = { x = 0.25, y = 0.27, z = 0.32, w = 1 },
				outlineColor = { x = 0.55, y = 0.6, z = 0.7, w = 1 },
				outlineThickness = 2,
			})
		end
	end,
})

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

-- Update: now and then, a planet of a random size passes, the closer the bigger and the faster.
engine.ecs.system("game.SpawnPlanets", {
	phase = "Update",
	side = "client",
	query = {},
	run = function(ctx)
		countdown -= ctx.time.delta
		if countdown > 0 then
			return
		end
		countdown = planetDelayMin + math.random() * (planetDelayMax - planetDelayMin)
		local planet = assets.planets[math.random(1, #assets.planets)]
		local closeness = math.random()
		local scale = backgroundScale * math.lerp(planetScaleMin, 1, closeness)
		local speed = backgroundScale * math.lerp(planetSpeedMin, planetSpeedMax, closeness)
		local width, height = planet.size.x * scale, planet.size.y * scale
		ctx.commands:spawn({
			["aylo.Transform"] = {
				position = { x = Duel.viewWidth, y = math.random() * (Duel.viewHeight - height) },
				scale = { x = scale, y = scale },
			},
			["aylo.SpriteRenderer"] = {
				texture = planet.texture,
				source = { x = 0, y = 0, z = planet.size.x, w = planet.size.y }, -- the whole image
				layer = planetDrawLayer,
			},
			["aylo.Parallax"] = { scroll = { x = -speed, y = 0 } },
			["game.BackgroundPlanet"] = { width = width },
		})
	end,
})

-- Update: removes the planets that left the view on the left.
engine.ecs.system("game.RemovePlanets", {
	phase = "Update",
	side = "client",
	query = { read = { "aylo.Transform", "aylo.Parallax", "game.BackgroundPlanet" } },
	after = { "game.FollowLasers" },
	run = function(ctx)
		for entity, transform, parallax, planet in ctx.query do
			if transform.position.x + parallax.scrolled.x + planet.width < 0 then
				ctx.commands:despawn(entity)
			end
		end
	end,
})
