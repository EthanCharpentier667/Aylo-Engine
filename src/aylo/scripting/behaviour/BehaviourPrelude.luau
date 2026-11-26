-- engine.behaviour: a script attached to entities, written in the Unity style, and compiled to what the ECS already
-- does (docs/scripting/dx-plan.md, D1 and D2): its fields are a component of its name, and each hook it defines is a
-- system named `<name>.<hook>` that calls the hook once per entity having the component.
--
--   local Spinner = engine.behaviour("game.Spinner", { fields = { speed = "f32" }, side = "client" })
--   function Spinner:onUpdate(dt)
--       local transform = self:getComponent("aylo.Transform2D")
--       transform.rotation += self.speed * dt
--   end
--   return Spinner
--
-- `onStart` is the system `<name>.onStart`, in the PreUpdate phase: it visits the entities without the tag
-- `<name>.Started`, calls the hook and gives them the tag, so it runs once per entity, and the other hooks skip an
-- entity until it has the tag. The state stays in the world: a hot reload does not start the entities again.
local forms = engine.forms
local declareComponent = engine.ecs.component
local declareSystem = engine.ecs.system

-- The hooks a Behaviour can define, with the phase of their system and the time step they receive.
local hookSpecs = {
	onStart = { phase = "PreUpdate" },
	onUpdate = { phase = "Update", delta = "delta" },
	onFixedUpdate = { phase = "FixedUpdate", delta = "fixedDelta" },
}
local hookList = "onStart, onUpdate, onFixedUpdate"

local optionKeys = { "fields", "defaults", "replication", "presence", "side" }

-- The presence of the tag `<name>.Started` for each side: it lives only where the hooks run, and is never sent.
local startedPresence = { server = "serverOnly", client = "clientOnly", both = "both" }

-- What a self or a handle holds (ctx, entity, and the view of a self), under a key no script can name.
local STATE = {}

local methods = {}

-- Fails unless `object` is a self or a handle: the usual cause is a method called with `.` instead of `:`.
local function stateOf(object, method)
	local state = type(object) == "table" and rawget(object, STATE)
	if not state then
		error(method .. " is a method: call it with ':', as in self:" .. method .. "(...)", 3)
	end
	return state
end

-- Fails unless `name` is the name of a component.
local function checkName(method, name)
	if type(name) ~= "string" then
		error(method .. " expects the name of a component: self:" .. method .. "(\"game.Health\")", 3)
	end
end

local handleMeta = {}

-- A handle on another entity: it has the methods of a self, and no fields.
local function newHandle(ctx, entity)
	return setmetatable({ [STATE] = { ctx = ctx, entity = entity } }, handleMeta)
end

-- The entity of a self or a handle, or nil for any other value.
local function entityOf(value)
	local state = type(value) == "table" and rawget(value, STATE)
	return state and state.entity
end

function methods.getComponent(object, name)
	local state = stateOf(object, "getComponent")
	checkName("getComponent", name)
	return state.ctx:get(state.entity, name)
end

function methods.hasComponent(object, name)
	local state = stateOf(object, "hasComponent")
	checkName("hasComponent", name)
	return state.ctx:has(state.entity, name)
end

function methods.getParent(object)
	local state = stateOf(object, "getParent")
	local parent = state.ctx:getParent(state.entity)
	return parent and newHandle(state.ctx, parent)
end

function methods.getChildren(object)
	local state = stateOf(object, "getChildren")
	local children = state.ctx:getChildren(state.entity)
	for index, child in children do
		children[index] = newHandle(state.ctx, child)
	end
	return children
end

function methods.getHandle(object, entity)
	local state = stateOf(object, "getHandle")
	if typeof(entity) ~= "Entity" then
		error("getHandle expects an entity: self:getHandle(collision.other)", 2)
	end
	return newHandle(state.ctx, entity)
end

function methods.addComponent(object, name, fields)
	local state = stateOf(object, "addComponent")
	checkName("addComponent", name)
	state.ctx.commands:add(state.entity, name, fields or {})
end

function methods.removeComponent(object, name)
	local state = stateOf(object, "removeComponent")
	checkName("removeComponent", name)
	state.ctx.commands:remove(state.entity, name)
end

function methods.destroy(object)
	local state = stateOf(object, "destroy")
	state.ctx.commands:despawn(state.entity)
end

-- Names a field cannot take: those a self already answers.
local reserved = { entity = true, ctx = true }
for method in methods do
	reserved[method] = true
end
for hook in hookSpecs do
	reserved[hook] = true
end

handleMeta.__index = function(object, key)
	if key == "entity" then
		return rawget(object, STATE).entity
	end
	local method = methods[key]
	if method == nil then
		error("an entity handle has no '" .. tostring(key) .. "': read its components with getComponent", 2)
	end
	return method
end
handleMeta.__newindex = function(_, key)
	error("cannot write '" .. tostring(key) .. "' on an entity handle: write the fields of its components", 2)
end
handleMeta.__eq = function(left, right)
	return entityOf(left) == entityOf(right)
end
handleMeta.__tostring = function(object)
	return "Handle(" .. tostring(rawget(object, STATE).entity) .. ")"
end

-- The metatable of the selves of one Behaviour: its fields read and write the view of its component.
local function newSelfMeta(name, fields, behaviour)
	local meta = {}
	meta.__index = function(object, key)
		local state = rawget(object, STATE)
		if fields[key] then
			return state.view[key]
		end
		if key == "entity" then
			return state.entity
		end
		if key == "ctx" then
			return state.ctx
		end
		local method = methods[key]
		if method ~= nil then
			return method
		end
		local own = behaviour[key]
		if own ~= nil then
			return own
		end
		error("'" .. name .. "' has no field '" .. tostring(key) .. "'", 2)
	end
	meta.__newindex = function(object, key, value)
		if typeof(value) == "ComponentView" then
			error(
				"cannot keep a view in self."
					.. tostring(key)
					.. ": a view only lives during the hook, call self:getComponent in each hook",
				2
			)
		end
		if not fields[key] then
			error(
				"'"
					.. name
					.. "' has no field '"
					.. tostring(key)
					.. "': a Behaviour keeps its state in its declared fields, add it to 'fields'",
				2
			)
		end
		rawget(object, STATE).view[key] = entityOf(value) or value
	end
	meta.__eq = handleMeta.__eq
	meta.__tostring = function(object)
		return name .. "(" .. tostring(rawget(object, STATE).entity) .. ")"
	end
	return meta
end

function engine.behaviour(name, spec)
	if type(name) ~= "string" then
		forms.fail("engine.behaviour", "expected the name of the Behaviour first, such as 'game.Spinner'")
	end
	local context = "engine.behaviour '" .. name .. "'"
	forms.table(context, spec, optionKeys)
	if type(spec.fields) ~= "table" then
		forms.fail(context, "'fields' is required: a table of field name to type, such as { speed = \"f32\" }")
	end
	local fields = {}
	for field in spec.fields do
		if reserved[field] then
			forms.fail(context, "the field '" .. tostring(field) .. "' has the name of a method of self: rename it")
		end
		fields[field] = true
	end
	local side = forms.string(context, spec, "side") or "server"
	if startedPresence[side] == nil then
		forms.fail(context, "'side' must be \"server\", \"client\" or \"both\"")
	end
	declareComponent(name, {
		fields = spec.fields,
		defaults = spec.defaults,
		replication = spec.replication,
		presence = spec.presence,
	})

	-- A client may not write a replicated component: on that side, the hooks read the fields.
	local replicated = spec.replication ~= nil and spec.replication ~= "none"
	local access = (side ~= "server" and replicated) and "read" or "write"
	local started = name .. ".Started"
	local hooks = {}
	local behaviour = {}
	local selfMeta = newSelfMeta(name, fields, behaviour)

	local function newSelf(ctx, entity, view)
		return setmetatable({ [STATE] = { ctx = ctx, entity = entity, view = view } }, selfMeta)
	end

	local function declareStart()
		declareComponent(started, { fields = {}, presence = startedPresence[side] })
		declareSystem(name .. ".onStart", {
			phase = hookSpecs.onStart.phase,
			side = side,
			query = { [access] = { name }, without = { started } },
			run = function(ctx)
				for entity, view in ctx.query do
					-- Tagged first: a hook that fails is not called again on every frame.
					ctx.commands:add(entity, started, {})
					hooks.onStart(newSelf(ctx, entity, view))
				end
			end,
		})
	end

	local function declareHook(hook)
		local delta = hookSpecs[hook].delta
		declareSystem(name .. "." .. hook, {
			phase = hookSpecs[hook].phase,
			side = side,
			query = { [access] = { name } },
			run = function(ctx)
				local call = hooks[hook]
				local waitsForStart = hooks.onStart ~= nil
				local dt = ctx[delta]
				for entity, view in ctx.query do
					if not waitsForStart or ctx:has(entity, started) then
						call(newSelf(ctx, entity, view), dt)
					end
				end
			end,
		})
	end

	return setmetatable(behaviour, {
		__index = hooks,
		__newindex = function(_, key, value)
			if type(key) == "string" and string.match(key, "^on%u") then
				if hookSpecs[key] == nil then
					error(
						context
							.. ": '"
							.. key
							.. "' is not a hook (hooks are "
							.. hookList
							.. "); name a helper without the 'on' prefix",
						2
					)
				end
				if type(value) ~= "function" then
					error(context .. ": '" .. key .. "' must be a function", 2)
				end
				if hooks[key] ~= nil then
					error(context .. ": '" .. key .. "' is defined twice", 2)
				end
				hooks[key] = value
				if key == "onStart" then
					declareStart()
				else
					declareHook(key)
				end
				return
			end
			if fields[key] or reserved[key] then
				error(context .. ": '" .. tostring(key) .. "' is a field or a method of self, a function cannot take its name", 2)
			end
			rawset(behaviour, key, value)
		end,
		__tostring = function()
			return "Behaviour(" .. name .. ")"
		end,
	})
end
