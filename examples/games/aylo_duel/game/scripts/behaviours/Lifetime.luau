-- Removes its entity once it lived `seconds`, on the server: a Misfire (actors/misfire.actor) lives just long enough
-- for a snapshot to carry it to the clients.
local Lifetime = engine.behaviour("game.Lifetime", {
	fields = { seconds = "f32" }, -- left to live
	defaults = { seconds = 1 },
	presence = "serverOnly",
	side = "server",
})

function Lifetime:onFixedUpdate(dt)
	self.seconds -= dt
	if self.seconds <= 0 then
		self:destroy()
	end
end

return Lifetime
