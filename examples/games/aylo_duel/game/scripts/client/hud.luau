-- The texts over the game: each player's name at the top of their half, a line at the bottom that says where the round
-- is (it follows the server's game.Round), and who won when it is over.

local assets = require("@scripts/client/assets")
local Duel = require("@scripts/shared/arena")

local textLayer = 10 -- over every sprite
local nameSize, nameTop = 22, 12
local nameColor = { x = 1, y = 1, z = 1, w = 0.85 }
local myNameColor = { x = 1, y = 0.85, z = 0.3, w = 1 }
local bannerSize, bannerY = 20, 572
local bannerColor = { x = 0.7, y = 0.75, z = 0.9, w = 1 }
-- Shapes are drawn over every text, so the end of the round is written in the gaps of the middle column: who won
-- between the top wall and the rock, and whether it is this player between the rock and the bottom wall
-- (assets/scenes/arena.scene).
local winnerSize = 40
local winnerY, verdictY = 237, 363
local winColor = { x = 1, y = 0.85, z = 0.3, w = 1 }
local loseColor = { x = 1, y = 0.45, z = 0.4, w = 1 }

-- A player's name over their half.
engine.ecs.component("game.PlayerName", { fields = { player = "u32" }, presence = "clientOnly" })
-- The line at the bottom about the round.
engine.ecs.component("game.Banner", { fields = {}, presence = "clientOnly" })
-- A text of the end of a round, removed when the next one starts.
engine.ecs.component("game.Announcement", { fields = {}, presence = "clientOnly" })

--- A text in the arena, centred on @p position.
local function text(content: string, position, size: number, color)
	return {
		["aylo.Transform2D"] = { position = position },
		["aylo.TextRenderer"] = {
			text = content,
			font = assets.font,
			size = size,
			color = color,
			anchor = { x = 0.5, y = 0.5 },
			layer = textLayer,
		},
	}
end

-- Startup: each player's name, at the top of the middle of their half, and the empty banner.
engine.ecs.system("game.SpawnNames", {
	phase = "Startup",
	side = "client",
	run = function(ctx)
		for player = 0, Duel.playerCount - 1 do
			local name = text(`Player {player + 1}`, { x = Duel.viewWidth / 4 * (2 * player + 1), y = nameTop }, nameSize, nameColor)
			name["aylo.TextRenderer"].anchor = { x = 0.5, y = 0 }
			name["game.PlayerName"] = { player = player }
			ctx.commands:spawn(name)
		end
		local banner = text("", { x = Duel.viewWidth / 2, y = bannerY }, bannerSize, bannerColor)
		banner["game.Banner"] = {}
		ctx.commands:spawn(banner)
	end,
})

-- Who is who and where the round is come from the network: run alone (the editor), there are no players.
if not Duel.networked then
	return
end

-- The ships' players by their client's id, filled by game.TrackOwners.
local playerOf: { [number]: number } = {}

-- Update: the ships' players by client, for the names and game.AnnounceWinner.
engine.ecs.system("game.TrackOwners", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Ship", "net.Owner" } },
	before = { "game.AnnounceWinner" },
	run = function(ctx)
		table.clear(playerOf)
		for _, ship, owner in ctx.query do
			playerOf[owner.client] = ship.player
		end
	end,
})

-- Update: this player's name says it is them.
engine.ecs.system("game.MarkMyName", {
	phase = "Update",
	side = "client",
	query = { read = { "game.PlayerName" }, write = { "aylo.TextRenderer" } },
	calls = { "aylo.replication.localClient" },
	after = { "game.TrackOwners", "game.ShowServerStatus" },
	run = function(ctx)
		local me = ctx.call("aylo.replication.localClient", {})
		local mine = if me.connected then playerOf[me.client] else nil
		for _, name, renderer in ctx.query do
			local isMe = name.player == mine
			renderer.text = if isMe then `Player {name.player + 1} (you)` else `Player {name.player + 1}`
			renderer.color = if isMe then myNameColor else nameColor
		end
	end,
})

-- Seconds before the next round, counted from the moment the server said the round is over.
local nextRoundIn = 0

-- On entering Over (the server's change, followed here): the clock of the next round starts.
engine.ecs.system("game.StartNextRoundClock", {
	onEnter = { "game.Round", "Over" },
	side = "client",
	run = function()
		nextRoundIn = Duel.rematchDelay
	end,
})

-- Update: the banner says where the round is, while this player plays.
engine.ecs.system("game.ShowBanner", {
	phase = "Update",
	side = "client",
	query = { write = { "aylo.TextRenderer" }, with = { "game.Banner" } },
	calls = { "aylo.state.get" },
	after = { "game.AnnounceWinner" },
	run = function(ctx)
		local playing = ctx.call("aylo.state.get", { state = "game.Screen" }) == "Playing"
		local round = ctx.call("aylo.state.get", { state = "game.Round" })
		local line = ""
		if round == "Over" then
			nextRoundIn = math.max(nextRoundIn - ctx.time.delta, 0)
			line = `Next round in {math.max(math.ceil(nextRoundIn), 1)}`
		elseif round == "Waiting" then
			line = "Waiting for an opponent..."
		end
		for _, renderer in ctx.query do
			renderer.text = if playing then line else ""
		end
	end,
})

-- Update: once the server spawns the Winner, says in the middle of the arena who won, and whether it is this player.
engine.ecs.system("game.AnnounceWinner", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Winner" }, added = { "game.Winner" } },
	calls = { "aylo.replication.localClient" },
	after = { "game.MarkMyName" },
	run = function(ctx)
		for _, winner in ctx.query do
			local me = ctx.call("aylo.replication.localClient", {})
			local seat = if me.connected then playerOf[me.client] else nil
			local lines = { { text = `Player {winner.player + 1} wins!`, y = winnerY } }
			if seat ~= nil then -- a player in the menu only watches
				table.insert(lines, { text = if seat == winner.player then "You win :)" else "You lose :(", y = verdictY })
			end
			local color = if seat == nil or seat == winner.player then winColor else loseColor
			for _, line in lines do
				local announcement = text(line.text, { x = Duel.viewWidth / 2, y = line.y }, winnerSize, color)
				announcement["game.Announcement"] = {}
				ctx.commands:spawn(announcement)
			end
		end
	end,
})

-- On leaving Over: the texts of the last round go.
engine.ecs.system("game.ClearAnnouncements", {
	onExit = { "game.Round", "Over" },
	side = "client",
	query = { with = { "game.Announcement" } },
	run = function(ctx)
		for entity in ctx.query do
			ctx.commands:despawn(entity)
		end
	end,
})
