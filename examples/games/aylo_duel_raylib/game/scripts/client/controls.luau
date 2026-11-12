-- Escape quits. The action is the client's own: added by a call rather than by engine.input.action, it stays out of
-- the inputs sent to the server.

print("WASD or the arrows to move, click to fire, F3 for the colliders, Escape to quit.")

engine.call("aylo.input.addAction", { name = "quit", type = "button" })
engine.call("aylo.input.bind", { action = "quit", control = "Escape" })

-- Update: Escape closes the game.
engine.ecs.system("game.QuitOnEscape", {
	phase = "Update",
	side = "client",
	query = {},
	calls = { "aylo.input.read" },
	events = { send = { "aylo.AppExit" } },
	run = function(ctx)
		if ctx.call("aylo.input.read", { action = "quit" }).pressed then
			ctx.events.send("aylo.AppExit", { code = 0 })
		end
	end,
})
