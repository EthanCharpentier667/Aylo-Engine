-- A planet passing behind the arena (actors/planet_ring.actor, planet_big.actor), on the clients. It starts on the
-- right of the view, at a random height and distance: the closer, the bigger and the faster. It goes once it left the
-- view on the left. scripts/client/scene.luau spawns one now and then.
local Duel = require("@scripts/shared/arena")

local backgroundScale = Duel.viewHeight / 160 -- the background's images are 160 pixels high, scaled to the view
local scaleMin = 0.6 -- the size of the farthest planet, from 1 for the closest...
local speedMin, speedMax = 22, 34 -- ... and its speed, in background pixels per second

local Planet = engine.behaviour("game.BackgroundPlanet", {
	fields = {
		size = "vec2", -- its image, in pixels
		width = "f32", -- on screen, in world units, once it started
	},
	presence = "clientOnly",
	side = "client",
})

function Planet:onStart()
	local closeness = math.random()
	local scale = backgroundScale * math.lerp(scaleMin, 1, closeness)
	self.width = self.size.x * scale
	local transform = self:getComponent("aylo.Transform2D")
	transform.position = { x = Duel.viewWidth, y = math.random() * (Duel.viewHeight - self.size.y * scale) }
	transform.scale = { x = scale, y = scale }
	local parallax = self:getComponent("aylo.Parallax")
	parallax.scroll = { x = -backgroundScale * math.lerp(speedMin, speedMax, closeness), y = 0 }
end

function Planet:onUpdate()
	local transform = self:getComponent("aylo.Transform2D")
	local parallax = self:getComponent("aylo.Parallax")
	if transform.position.x + parallax.scrolled.x + self.width < 0 then
		self:destroy()
	end
end

return Planet
