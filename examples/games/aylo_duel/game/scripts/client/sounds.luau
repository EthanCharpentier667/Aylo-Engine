-- Each shot sounds a little different: the laser's and the misfire's sound come with their actors
-- (assets/actors/laser.actor and misfire.actor), and this draws their pitch and volume at random. The music is a node
-- of the arena (assets/scenes/arena.scene).

local shotPitchMin, shotPitchMax = 0.9, 1.1
local shotVolumeMin, shotVolumeMax = 0.8, 1.0

-- Update: a shot's sound gets its own pitch and volume as soon as it starts.
for index, shot in { "game.Laser", "game.Misfire" } do
	engine.ecs.system(`game.Vary{string.sub(shot, 6)}Sounds`, {
		phase = "Update",
		side = "client",
		query = { write = { "aylo.AudioSource" }, with = { shot }, added = { "aylo.AudioSource" } },
		after = if index == 2 then { "game.VaryLaserSounds" } else nil,
		run = function(ctx)
			for _, sound in ctx.query do
				sound.volume = math.lerp(shotVolumeMin, shotVolumeMax, math.random())
				sound.pitch = math.lerp(shotPitchMin, shotPitchMax, math.random())
			end
		end,
	})
end
