-- What both sides declare alike: the replicated components and the input actions (the manifest checks that the server
-- and the clients declared the same ones), the round, the messages of the quick join, and the components the capsules
-- name (every side reads every capsule file). Required by shared/main.luau, which every process runs first. The
-- Behaviours (scripts/behaviours/) run on every side by themselves.

local Duel = require("@scripts/shared/arena")

-- The round, the server's: Waiting until both seats are taken, Live until a laser hits, Over while the clients show who
-- won, then Waiting again for the next round. The clients follow it (server/round.luau changes it).
engine.ecs.state("game.Round", { "Waiting", "Live", "Over" }, { replicate = true })

-- The quick join: a client in the main menu asks for a seat, and the server answers with the seat, or says that the
-- arena is full. A player who goes back to the menu gives their seat up.
engine.net.packet("game.Join", { direction = "toServer", fields = {} })
engine.net.packet("game.Leave", { direction = "toServer", fields = {} })
engine.net.packet("game.Seat", {
	direction = "toClients",
	fields = { { "granted", "bool" }, { "player", "u8" } }, -- player: 0 on the left, 1 on the right; 0 when refused
})

-- A player's ship, on every change.
engine.ecs.component("game.Ship", {
	fields = { position = "vec2", player = "u32" }, -- player 0 is on the left, firing right; 1 on the right
	replication = "everyChange",
})
-- A laser in flight: it flies away from its shooter and cannot hit them.
engine.ecs.component("game.Laser", {
	fields = { position = "vec2", shooter = "u32" },
	replication = "everyChange",
})
-- Spawned by the server when a laser hits: the game is over.
engine.ecs.component("game.Winner", { fields = { player = "u32" }, replication = "everyChange" })
-- Spawned by the server instead of a laser when a shot misfires (actors/misfire.actor), so that the clients play the
-- misfire sound.
engine.ecs.component("game.Misfire", {
	fields = { position = "vec2", shooter = "u32" },
	replication = "everyChange",
})

-- The texts of the arena's scene, on the clients (scripts/client/hud.luau writes them): a player's name over their half,
-- and the line at the bottom about the round.
engine.ecs.component("game.PlayerName", { fields = { player = "u32" }, presence = "clientOnly" })
engine.ecs.component("game.Banner", { fields = {}, presence = "clientOnly" })

-- The actions both sides agree on: the clients send them, the server reads them.
engine.input.action("move", "vector2", { -- up and down (only Y is used)
	bind = {
		{ up = "W", down = "S", left = "A", right = "D" },
		{ up = "Up", down = "Down", left = "Left", right = "Right" },
	},
})
engine.input.action("shoot", "button", { bind = { "MouseLeft" } })

-- Startup, on every side: the arena (assets/scenes/arena.scene). Each side spawns the obstacles; the clients also spawn
-- the camera and the texts.
engine.ecs.system("game.LoadArena", {
	phase = "Startup",
	side = "both",
	query = {},
	calls = { "aylo.capsule.load" },
	run = function(ctx)
		engine.capsule.load(ctx, Duel.arena)
	end,
})
