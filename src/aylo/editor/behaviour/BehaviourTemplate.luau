-- The Behaviour game.@Name@. Give it to a node in the Inspector: Add component, Behaviours.
-- Its fields are what the Inspector shows and a capsule saves. See the manual's Behaviours page.
local @Name@ = engine.behaviour("game.@Name@", {
	fields = { speed = "f32" },
	defaults = { speed = 1 },
	side = "server", -- "client" for what only the players see, "both" for both
})

-- Once per entity, before its first update.
function @Name@:onStart()
end

-- Every fixed tick, where the gameplay of the server goes. dt is the fixed step, in seconds.
function @Name@:onFixedUpdate(dt)
	-- local transform = self:getComponent("aylo.Transform2D")
	-- transform.rotation += self.speed * dt
end

-- Write the hooks you need; each one costs a call per entity. Remove the `--[[` and `]]` around one to use it.
--[[
-- Every frame, for what the players see (side = "client"): a dedicated server runs no frame update.
function @Name@:onUpdate(dt)
end

-- When the entity is switched on (and once at the start, before onStart).
function @Name@:onEnable()
end

-- When the entity, or one of its parents, is switched off (aylo.Disabled).
function @Name@:onDisable()
end

-- Once the entity lost the Behaviour or was despawned: its fields are gone.
function @Name@:onDestroy()
end
]]

return @Name@
