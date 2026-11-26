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
-- The lifecycle is kept in the world, with tags the hooks' side has and never sends, so a hot reload neither starts nor
-- enables the entities again:
-- - `onStart` (PreUpdate) visits the entities without `<name>.Started`, tags them and calls the hook; the other hooks
--   skip an entity until it has the tag.
-- - `onEnable` (PreUpdate) visits the enabled entities without `<name>.Active` and tags them; `onDisable` (PreUpdate)
--   visits the tagged ones `ctx:isEnabled` finds switched off and takes the tag away. They come together: each tracks
--   what the other needs.
-- - `onDestroy` (PostUpdate) visits the entities that lost the component (`removed`), and takes the tags away. It is
--   there as soon as a tag is, to keep a component given back from looking started.
local forms = engine.forms
local declareComponent = engine.ecs.component
local declareSystem = engine.ecs.system

-- The hooks a Behaviour can define, with the phase of their system and the time step they receive.
local hookSpecs = {
	onStart = { phase = "PreUpdate" },
	onEnable = { phase = "PreUpdate" },
	onDisable = { phase = "PreUpdate" },
	onUpdate = { phase = "Update", delta = "delta" },
	onFixedUpdate = { phase = "FixedUpdate", delta = "fixedDelta" },
	onDestroy = { phase = "PostUpdate" },
}
-- The order of the systems of the lifecycle in PreUpdate.
local lifecycleOrder = { "onDisable", "onEnable", "onStart" }
local hookList = "onStart, onEnable, onDisable, onUpdate, onFixedUpdate, onDestroy"

local optionKeys = { "fields", "defaults", "replication", "presence", "side" }

-- The presence of the tags `<name>.Started` and `<name>.Active` for each side: they live only where the hooks run.
local tagPresence = { server = "serverOnly", client = "clientOnly", both = "both" }

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
	-- The view of the self of `state`; in onDestroy, there is none.
	local function viewOf(state, key)
		if state.view == nil then
			error(
				"cannot use self."
					.. tostring(key)
					.. ": in onDestroy, the entity no longer has '"
					.. name
					.. "' and its fields are gone",
				3
			)
		end
		return state.view
	end
	local meta = {}
	meta.__index = function(object, key)
		local state = rawget(object, STATE)
		if fields[key] then
			return viewOf(state, key)[key]
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
		viewOf(rawget(object, STATE), key)[key] = entityOf(value) or value
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
	if tagPresence[side] == nil then
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
	local active = name .. ".Active"
	local hooks = {}
	local declared = {} -- the systems declared, by hook
	local tags = {} -- the tags declared, by name
	local behaviour = {}
	local selfMeta = newSelfMeta(name, fields, behaviour)

	local function newSelf(ctx, entity, view)
		return setmetatable({ [STATE] = { ctx = ctx, entity = entity, view = view } }, selfMeta)
	end

	local function system(hook, options)
		declared[hook] = true
		options.phase = hookSpecs[hook].phase
		options.side = side
		declareSystem(name .. "." .. hook, options)
	end

	local declareDestroy

	local function declareTag(tag)
		if not tags[tag] then
			tags[tag] = true
			declareComponent(tag, { fields = {}, presence = tagPresence[side] })
			declareDestroy()
		end
	end

	-- `<name>.onDestroy`: what lost the component. An entity that has it again (taken away and given back between two
	-- runs) is not destroyed.
	function declareDestroy()
		if declared.onDestroy then
			return
		end
		system("onDestroy", {
			-- Names aylo.Disabled, so a switched-off entity that loses the component is visited too.
			query = { removed = { name }, optionalRead = { "aylo.Disabled" } },
			run = function(ctx)
				for entity in ctx.query do
					if not ctx:has(entity, name) then
						for tag in tags do
							ctx.commands:remove(entity, tag)
						end
						if hooks.onDestroy then
							hooks.onDestroy(newSelf(ctx, entity, nil))
						end
					end
				end
			end,
		})
	end

	-- Orders the PreUpdate systems of the lifecycle, onDisable, onEnable, then onStart, whichever is written first.
	local function orderLifecycle(options, hook)
		local after, before = {}, {}
		local seen = false
		for _, other in lifecycleOrder do
			if other == hook then
				seen = true
			elseif declared[other] then
				table.insert(seen and before or after, name .. "." .. other)
			end
		end
		options.after = #after > 0 and after or nil
		options.before = #before > 0 and before or nil
		return options
	end

	local function declareStart()
		declareTag(started)
		system(
			"onStart",
			orderLifecycle({
				query = { [access] = { name }, without = { started } },
				run = function(ctx)
					for entity, view in ctx.query do
						-- Tagged first: a hook that fails is not called again on every frame.
						ctx.commands:add(entity, started, {})
						hooks.onStart(newSelf(ctx, entity, view))
					end
				end,
			}, "onStart")
		)
	end

	-- `<name>.onEnable` and `<name>.onDisable`, declared together with the tag `<name>.Active`.
	local function declareActivity()
		if declared.onEnable then
			return
		end
		declareTag(active)
		system(
			"onEnable",
			orderLifecycle({
				query = { [access] = { name }, without = { active } }, -- only the enabled ones: the query skips the others
				run = function(ctx)
					for entity, view in ctx.query do
						ctx.commands:add(entity, active, {})
						if hooks.onEnable then
							hooks.onEnable(newSelf(ctx, entity, view))
						end
					end
				end,
			}, "onEnable")
		)
		system("onDisable", orderLifecycle({
			query = { [access] = { name }, with = { active }, optionalRead = { "aylo.Disabled" } },
			run = function(ctx)
				for entity, view in ctx.query do
					if not ctx:isEnabled(entity) then
						ctx.commands:remove(entity, active)
						if hooks.onDisable then
							hooks.onDisable(newSelf(ctx, entity, view))
						end
					end
				end
			end,
		}, "onDisable"))
	end

	local function declareHook(hook)
		local delta = hookSpecs[hook].delta
		system(hook, {
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

	local declareFor = {
		onStart = declareStart,
		onEnable = declareActivity,
		onDisable = declareActivity,
		onDestroy = declareDestroy,
	}

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
				local declare = declareFor[key]
				if declare then
					declare()
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
