-- Escape goes back one screen (client/menu.luau reads it). The action is the client's own: added by a call rather than
-- by engine.input.action, it stays out of the inputs sent to the server.

print("WASD or the arrows to move, click to fire, F3 for the colliders, Escape to go back.")

engine.call("aylo.input.addAction", { name = "back", type = "button" })
engine.call("aylo.input.bind", { action = "back", control = "Escape" })
