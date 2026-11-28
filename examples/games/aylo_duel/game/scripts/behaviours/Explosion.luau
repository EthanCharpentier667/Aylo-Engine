-- An explosion (actors/explosion.actor), on the clients: it goes once its animation, played once, is over.
local Explosion = engine.behaviour("game.Explosion", {
	fields = {},
	presence = "clientOnly",
	side = "client",
})

function Explosion:onUpdate()
	local animator = self:getComponent("aylo.SpriteAnimator")
	if animator ~= nil and not animator.playing then
		self:destroy()
	end
end

return Explosion
