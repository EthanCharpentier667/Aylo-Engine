-- The main menu, over the arena (a match may be playing behind it): Quick join asks the server for a seat, Quit closes
-- the game. The screens are the client's own state machine, game.Screen: each spawns its interface when it is entered
-- and removes it when it is left.
--
--   Menu --Quick join--> Joining --the server gives a seat--> Playing --Escape--> Menu
--                           \--full, no server, Cancel--> Menu

local Duel = require("@scripts/shared/arena")

-- Run alone (the editor), there is no server to join: the game starts in Playing, and the menu never shows.
engine.ecs.state("game.Screen", if Duel.networked then { "Menu", "Joining", "Playing" } else { "Playing", "Menu", "Joining" })

-- The screens are capsule files (assets/ui/menu.capsule and joining.capsule, their panel and buttons in panel.capsule
-- and button.capsule): this module only spawns them and writes what changes while they show.
local spinnerFrames, spinnerFrameSize, spinnerFrameDuration = 11, 16, 0.06

local pale = { x = 0.7, y = 0.75, z = 0.9, w = 1 }
local warning = { x = 1, y = 0.55, z = 0.45, w = 1 }
local fine = { x = 0.55, y = 1, z = 0.65, w = 1 }

-- What the menu says when it opens again: why the last join failed, or nil.
local notice: string? = nil
-- Whether this client asked the server for a seat since it entered Joining, and how long it has been looking.
local asked = false
local looking = 0

-- On entering Menu, and on entering Joining: the screen's capsule, over the arena.
for value, path in Duel.screens do
	engine.ecs.system(`game.Show{value}`, {
		onEnter = { "game.Screen", value },
		side = "client",
		calls = { "aylo.capsule.spawn" },
		run = function(ctx)
			if value == "Joining" then
				asked = false
				looking = 0
				notice = nil
			end
			engine.capsule.spawn(ctx, path)
		end,
	})
end

-- On leaving Menu, and on leaving Joining: the screen goes, its root with all its nodes.
for _, value in { "Menu", "Joining" } do
	engine.ecs.system(`game.Hide{value}`, {
		onExit = { "game.Screen", value },
		side = "client",
		query = { with = { "game.ScreenNode" } },
		after = if value == "Joining" then { "game.HideMenu" } else nil,
		run = function(ctx)
			for entity in ctx.query do
				ctx.commands:despawn(entity)
			end
		end,
	})
end

-- Update: the buttons of the screens take the image of their state (game.ButtonSkin): at rest, under the pointer or
-- focused, held.
engine.ecs.system("game.SkinButtons", {
	phase = "Update",
	side = "client",
	query = { read = { "aylo.UiInteraction", "game.ButtonSkin" }, write = { "aylo.UiImage" } },
	run = function(ctx)
		for _, interaction, skin, image in ctx.query do
			image.texture = if interaction.pressed
				then skin.pressed
				elseif interaction.hovered or interaction.focused then skin.hovered
				else skin.normal
		end
	end,
})

-- Update: the loading wheel turns.
engine.ecs.system("game.TurnSpinners", {
	phase = "Update",
	side = "client",
	query = { write = { "game.Spinner", "aylo.UiImage" } },
	after = { "game.SkinButtons" },
	run = function(ctx)
		for _, spinner, image in ctx.query do
			spinner.time += ctx.time.delta
			local frame = math.floor(spinner.time / spinnerFrameDuration) % spinnerFrames
			image.source = { x = frame * spinnerFrameSize, y = 0, z = spinnerFrameSize, w = spinnerFrameSize }
		end
	end,
})

-- Update, in the menu: says why the last join failed, or else whether the server answers.
engine.ecs.system("game.ShowServerStatus", {
	phase = "Update",
	side = "client",
	inState = { "game.Screen", "Menu" },
	query = { write = { "aylo.UiText" }, with = { "game.MenuStatus" } },
	calls = { "aylo.replication.localClient" },
	after = { "game.WatchServer" },
	run = function(ctx)
		local connected = engine.net.localClient(ctx).connected
		for _, text in ctx.query do
			if notice ~= nil then
				text.text = notice
				text.color = warning
			else
				text.text = if connected then "Server online" else "Looking for the server..."
				text.color = if connected then fine else pale
			end
		end
	end,
})

-- Update: the buttons, and Escape, which goes back one screen (from the menu, it quits).
engine.ecs.system("game.ReadMenu", {
	phase = "Update",
	side = "client",
	events = { read = { "aylo.UiClicked" }, send = { "aylo.AppExit" } },
	calls = { "aylo.input.read", "aylo.state.get", "aylo.state.set", "aylo.net.sendToServer" },
	run = function(ctx)
		local screenNow = engine.state.get(ctx, "game.Screen")
		local function go(value: string)
			engine.state.set(ctx, "game.Screen", value)
		end
		local action: string? = nil
		for click in ctx.events.read("aylo.UiClicked") do
			action = click.action
		end
		if action == nil and engine.input.read(ctx, "back").pressed then
			action = if screenNow == "Menu" then "menu.quit" elseif screenNow == "Joining" then "menu.cancel" else "menu.leave"
		end
		if action == "menu.quickJoin" and screenNow == "Menu" then
			go("Joining")
		elseif action == "menu.quit" then
			ctx.events.send("aylo.AppExit", { code = 0 })
		elseif action == "menu.cancel" and screenNow == "Joining" then
			if asked then -- the server may already have given the seat: give it back
				engine.net.sendToServer(ctx, "game.Leave", {})
			end
			go("Menu")
		elseif action == "menu.leave" and screenNow == "Playing" and Duel.networked then
			engine.net.sendToServer(ctx, "game.Leave", {})
			go("Menu")
		end
	end,
})

-- Update, while joining: asks for a seat as soon as the server answers, and plays once it gives one. With no server
-- after Duel.joinTimeout seconds, or no free seat, back to the menu, which says why.
engine.ecs.system("game.QuickJoin", {
	phase = "Update",
	side = "client",
	inState = { "game.Screen", "Joining" },
	events = { read = { "net.PacketReceived" } },
	calls = { "aylo.replication.localClient", "aylo.net.sendToServer", "aylo.state.set" },
	after = { "game.ReadMenu" },
	run = function(ctx)
		local function back(why: string)
			notice = why
			engine.state.set(ctx, "game.Screen", "Menu")
		end
		looking += ctx.time.delta
		if not asked then
			if engine.net.localClient(ctx).connected then
				asked = engine.net.sendToServer(ctx, "game.Join", {})
			elseif looking > Duel.joinTimeout then
				back("No server answered: is it running?")
				return
			end
		end
		for _, packet in engine.net.received(ctx) do
			if packet.name == "game.Seat" then
				if packet.values.granted then
					print(`[client] seated as player {packet.values.player + 1}`)
					engine.state.set(ctx, "game.Screen", "Playing")
				else
					back("The arena is full: try again later")
				end
			end
		end
	end,
})

-- Update, while playing: back to the menu if the server went away.
engine.ecs.system("game.WatchServer", {
	phase = "Update",
	side = "client",
	inState = { "game.Screen", "Playing" },
	calls = { "aylo.replication.localClient", "aylo.state.set" },
	after = { "game.QuickJoin" },
	run = function(ctx)
		if Duel.networked and not engine.net.localClient(ctx).connected then
			notice = "Lost the server"
			engine.state.set(ctx, "game.Screen", "Menu")
		end
	end,
})
