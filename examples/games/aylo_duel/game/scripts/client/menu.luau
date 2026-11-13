-- The main menu, over the arena (a match may be playing behind it): Quick join asks the server for a seat, Quit closes
-- the game. The screens are the client's own state machine, game.Screen: each spawns its interface when it is entered
-- and removes it when it is left.
--
--   Menu --Quick join--> Joining --the server gives a seat--> Playing --Escape--> Menu
--                           \--full, no server, Cancel--> Menu

local assets = require("@scripts/client/assets")
local Duel = require("@scripts/shared/arena")

-- Run alone (the editor), there is no server to join: the game starts in Playing, and the menu never shows.
engine.ecs.state("game.Screen", if Duel.networked then { "Menu", "Joining", "Playing" } else { "Playing", "Menu", "Joining" })

-- Every node of a screen, removed with it.
engine.ecs.component("game.ScreenNode", { fields = {}, presence = "clientOnly" })
-- The loading wheel, turning: seconds since it appeared.
engine.ecs.component("game.Spinner", { fields = { time = "f32" }, presence = "clientOnly" })
-- The line of the menu that says whether the server is there.
engine.ecs.component("game.MenuStatus", { fields = {}, presence = "clientOnly" })

local pixel = 3 -- view pixels per pixel of the UI pack's art
local titlePanelSlice = { 35, 26, 35, 4 } -- its borders, in its pixels: the roof is the top one
local buttonSlice = { 5, 6, 5, 6 }
local spinnerFrames, spinnerFrameSize, spinnerFrameDuration = 11, 16, 0.06

local white = { 1, 1, 1, 1 }
local gold = { 1, 0.85, 0.3, 1 }
local pale = { 0.7, 0.75, 0.9, 1 }
local warning = { 1, 0.55, 0.45, 1 }
local fine = { 0.55, 1, 0.65, 1 }

-- What the menu says when it opens again: why the last join failed, or nil.
local notice: string? = nil
-- Whether this client asked the server for a seat since it entered Joining, and how long it has been looking.
local asked = false
local looking = 0

--- Marks @p spec and all its children as part of the screen, so that leaving the screen removes them all.
local function screenNode(spec)
	spec.components = spec.components or {}
	spec.components["game.ScreenNode"] = {}
	for _, child in spec.children or {} do
		screenNode(child)
	end
	return spec
end

--- A line of text, which lets the pointer through to what is under it.
local function label(text: string, size: number, color, extra: { [string]: any }?)
	local spec = {
		style = { ignorePointer = true },
		text = { text = text, font = assets.font, size = size, color = color, align = "center" },
	}
	for key, value in extra or {} do
		spec[key] = value
	end
	return spec
end

--- A button of the UI pack, its look switched by game.SkinButtons: the tint stays white.
local function button(text: string, action: string)
	return {
		style = { width = "100%", height = 58, justify = "center", alignItems = "center" },
		image = { texture = assets.ui.button.normal, slice = buttonSlice, sliceScale = pixel },
		button = { action = action, normal = white, hovered = white, pressed = white },
		children = { label(text, 20, white) },
	}
end

--- A whole screen: the arena dimmed, and in its middle the panel with a roof (a planet in it) holding @p children.
local function screen(commands, children)
	local panelWidth = 440
	engine.ui.spawn(
		commands,
		screenNode({
			style = { width = "100%", height = "100%", justify = "center", alignItems = "center" },
			background = { color = { 0.01, 0.01, 0.04, 0.6 } },
			children = {
				{
					style = {
						width = panelWidth,
						paddingTop = titlePanelSlice[2] * pixel + 10,
						paddingX = 48,
						paddingBottom = 34,
						gap = 14,
						alignItems = "center",
					},
					image = { texture = assets.ui.titlePanel, slice = titlePanelSlice, sliceScale = pixel },
					children = {
						{
							style = {
								position = "absolute",
								top = 16,
								left = panelWidth / 2 - 24,
								width = 48,
								height = 48,
								ignorePointer = true,
							},
							image = { texture = assets.ui.planet },
						},
						table.unpack(children),
					},
				},
				label("WASD or the arrows to move, click to fire, Escape to go back", 13, pale, {
					style = { position = "absolute", bottom = 16, ignorePointer = true },
				}),
			},
		})
	)
end

-- On entering Menu: the title, Quick join and Quit, and a line about the server.
engine.ecs.system("game.ShowMenu", {
	onEnter = { "game.Screen", "Menu" },
	side = "client",
	run = function(ctx)
		screen(ctx.commands, {
			label("AYLO DUEL", 44, gold),
			label("One shot, one winner", 14, pale, { style = { marginBottom = 10, ignorePointer = true } }),
			button("Quick join", "menu.quickJoin"),
			button("Quit", "menu.quit"),
			label(notice or "", 14, if notice then warning else pale, {
				style = { marginTop = 6, ignorePointer = true },
				components = { ["game.MenuStatus"] = {} },
			}),
		})
	end,
})

-- On entering Joining: a turning wheel while the server finds a seat, and Cancel.
engine.ecs.system("game.ShowJoining", {
	onEnter = { "game.Screen", "Joining" },
	side = "client",
	run = function(ctx)
		asked = false
		looking = 0
		notice = nil
		screen(ctx.commands, {
			label("QUICK JOIN", 34, gold),
			{
				style = { width = spinnerFrameSize * pixel, height = spinnerFrameSize * pixel, marginY = 8, ignorePointer = true },
				image = {
					texture = assets.ui.loadingWheel,
					source = { 0, 0, spinnerFrameSize, spinnerFrameSize },
				},
				components = { ["game.Spinner"] = {} },
			},
			label("Looking for a free seat...", 16, pale),
			button("Cancel", "menu.cancel"),
		})
	end,
})

-- On leaving Menu, and on leaving Joining: the screen's interface goes.
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

-- Update: the buttons of the screens take the pack's look of their state: at rest, under the pointer or focused, held.
engine.ecs.system("game.SkinButtons", {
	phase = "Update",
	side = "client",
	query = { read = { "aylo.UiInteraction" }, write = { "aylo.UiImage" }, with = { "aylo.UiButton", "game.ScreenNode" } },
	run = function(ctx)
		for _, interaction, image in ctx.query do
			image.texture = if interaction.pressed
				then assets.ui.button.pressed
				elseif interaction.hovered or interaction.focused then assets.ui.button.hovered
				else assets.ui.button.normal
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

-- Update, in the menu: says whether the server answers, unless the menu opened on why the last join failed.
engine.ecs.system("game.ShowServerStatus", {
	phase = "Update",
	side = "client",
	inState = { "game.Screen", "Menu" },
	query = { write = { "aylo.UiText" }, with = { "game.MenuStatus" } },
	calls = { "aylo.replication.localClient" },
	after = { "game.WatchServer" },
	run = function(ctx)
		if notice ~= nil then
			return
		end
		local connected = ctx.call("aylo.replication.localClient", {}).connected
		for _, text in ctx.query do
			text.text = if connected then "Server online" else "Looking for the server..."
			text.color = if connected then { x = fine[1], y = fine[2], z = fine[3], w = 1 } else { x = pale[1], y = pale[2], z = pale[3], w = 1 }
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
		local screenNow = ctx.call("aylo.state.get", { state = "game.Screen" })
		local function go(value: string)
			ctx.call("aylo.state.set", { state = "game.Screen", value = value })
		end
		local action: string? = nil
		for click in ctx.events.read("aylo.UiClicked") do
			action = click.action
		end
		if action == nil and ctx.call("aylo.input.read", { action = "back" }).pressed then
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
			ctx.call("aylo.state.set", { state = "game.Screen", value = "Menu" })
		end
		looking += ctx.time.delta
		if not asked then
			if ctx.call("aylo.replication.localClient", {}).connected then
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
					ctx.call("aylo.state.set", { state = "game.Screen", value = "Playing" })
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
		if Duel.networked and not ctx.call("aylo.replication.localClient", {}).connected then
			notice = "Lost the server"
			ctx.call("aylo.state.set", { state = "game.Screen", value = "Menu" })
		end
	end,
})
