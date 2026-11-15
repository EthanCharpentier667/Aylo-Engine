-- engine.physics: the physics engine calls as plain functions, from a system that lists the call each form makes in
-- its `calls`. Each checks its arguments, and its errors start with the form's name.
--
--   local hit = engine.physics.raycast(ctx, transform.position, { x = 1, y = 0 }, 500, { ignore = entity })
--   if hit then print(hit.entity, hit.point, hit.normal, hit.distance) end
--   transform.position = engine.physics.moveAndSlide(ctx, entity, transform.position, { x = 50, y = 0 })
--   local moved = engine.physics.moveAndSlide3D(ctx, entity, transform.position, delta, { stepHeight = 0.3 })
--
-- A query takes its shape as a table of the fields of a Collider (2D) or a Collider3D (3D), or as an entity's collider
-- itself, read from a query. A 2D vector is a table { x, y }; a 3D one is a Luau vector or a table { x, y, z }.
local forms = engine.forms

engine.physics = {}

local filterKeys = { "mask", "ignore", "includeSensors" }
local shapeKeys = { "shape", "size", "radius", "halfHeight", "offset" }
-- A copy of a collider may carry its other fields: they are allowed, and not read.
local colliderKeys = { "shape", "size", "radius", "halfHeight", "offset", "sensor", "friction", "restitution", "layer", "mask" }

-- Fails unless `ctx` is the context a system's `run` received.
local function checkContext(form, ctx)
	if type(ctx) ~= "table" or type(ctx.call) ~= "function" then
		forms.fail(form, "expected the system's ctx first")
	end
end

-- Fails unless `value` is a 2D vector: a table { x, y }.
local function vec2(form, value, name)
	if type(value) ~= "table" or type(value.x) ~= "number" or type(value.y) ~= "number" then
		forms.fail(form, "'" .. name .. "' must be a table { x, y }")
	end
	return value
end

-- Fails unless `value` is a 3D vector: a Luau vector or a table { x, y, z }.
local function vec3(form, value, name)
	if type(value) == "vector" then
		return value
	end
	if type(value) ~= "table" or type(value.x) ~= "number" or type(value.y) ~= "number" or type(value.z) ~= "number" then
		forms.fail(form, "'" .. name .. "' must be a vector or a table { x, y, z }")
	end
	return value
end

-- Fails unless `value` is a number.
local function number(form, value, name)
	if type(value) ~= "number" then
		forms.fail(form, "'" .. name .. "' must be a number")
	end
	return value
end

-- Fails unless `value` is an entity (not nil).
local function entity(form, value)
	if value == nil then
		forms.fail(form, "expected the entity second")
	end
	return value
end

-- Copies the fields of `shape` the call reads into `fields`: a table of collider fields, or a collider read from a
-- query. Checks the keys of a plain table, so a misspelled field is not left out silently.
local function readShape(form, shape, fields)
	if type(shape) == "table" then
		forms.table(form .. ": the shape", shape, colliderKeys)
	elseif type(shape) ~= "userdata" then
		forms.fail(form, "expected the shape (the fields of a collider, or a collider) second")
	end
	for _, key in shapeKeys do
		local value = shape[key]
		if value ~= nil then
			fields[key] = value
		end
	end
	return fields
end

-- Copies the filter of `options` (mask, ignore, includeSensors), and the other keys of `extra`, into `fields`.
local function readOptions(form, options, fields, extra)
	local keys = table.clone(filterKeys)
	for _, key in extra or {} do
		table.insert(keys, key)
	end
	local given = forms.options(form .. ": the options", options, keys)
	for key, value in given do
		fields[key] = value
	end
	if given.mask ~= nil then
		forms.count(form .. ": the options", given, "mask")
	end
	if given.includeSensors ~= nil then
		forms.boolean(form .. ": the options", given, "includeSensors")
	end
	local rotation = given.rotation
	if
		rotation ~= nil
		and (
			type(rotation) ~= "table"
			or type(rotation.x) ~= "number"
			or type(rotation.y) ~= "number"
			or type(rotation.z) ~= "number"
			or type(rotation.w) ~= "number"
		)
	then
		forms.fail(form .. ": the options", "'rotation' must be a quaternion { x, y, z, w } (engine.math.quat)")
	end
	return fields
end

-- Makes the engine call `name` through `ctx`, its errors starting with the form.
local function call(form, ctx, name, fields)
	local done, result = pcall(ctx.call, name, fields)
	if not done then
		forms.fail(form, tostring(result))
	end
	return result
end

-- The hit of a ray or a shape cast, or nil when it hit nothing.
local function hitOf(result)
	if result.hit then
		return result
	end
	return nil
end

-- Returns a ray form: `form(ctx, origin, direction, maxDistance, options)`, the hit or nil.
local function rayForm(form, name, vector)
	return function(...)
		forms.arguments(form, 5, ...)
		local ctx, origin, direction, maxDistance, options = ...
		checkContext(form, ctx)
		local fields = {
			origin = vector(form, origin, "origin"),
			direction = vector(form, direction, "direction"),
			maxDistance = number(form, maxDistance, "maxDistance"),
		}
		return hitOf(call(form, ctx, name, readOptions(form, options, fields)))
	end
end

-- Returns a shape cast form: `form(ctx, shape, from, direction, maxDistance, options)`, the hit or nil.
local function castForm(form, name, vector, extra)
	return function(...)
		forms.arguments(form, 6, ...)
		local ctx, shape, from, direction, maxDistance, options = ...
		checkContext(form, ctx)
		local fields = readShape(form, shape, {})
		fields.from = vector(form, from, "from")
		fields.direction = vector(form, direction, "direction")
		fields.maxDistance = number(form, maxDistance, "maxDistance")
		return hitOf(call(form, ctx, name, readOptions(form, options, fields, extra)))
	end
end

-- Returns an overlap form: `form(ctx, shape, at, options)`, how many entities the shape covers and one of them.
local function overlapForm(form, name, vector, extra)
	return function(...)
		forms.arguments(form, 4, ...)
		local ctx, shape, at, options = ...
		checkContext(form, ctx)
		local fields = readShape(form, shape, {})
		fields.at = vector(form, at, "at")
		local result = call(form, ctx, name, readOptions(form, options, fields, extra))
		if result.count == 0 then
			return 0, nil
		end
		return result.count, result.first
	end
end

-- Returns a form that pushes an entity's body: `form(ctx, entity, value)`.
local function pushForm(form, name, read)
	return function(...)
		forms.arguments(form, 3, ...)
		local ctx, target, value = ...
		checkContext(form, ctx)
		call(form, ctx, name, { entity = entity(form, target), value = read(form, value, "value") })
	end
end

-- Returns a form that sets the gravity: `form(ctx, gravity)`.
local function gravityForm(form, name, vector)
	return function(...)
		forms.arguments(form, 2, ...)
		local ctx, gravity = ...
		checkContext(form, ctx)
		call(form, ctx, name, { gravity = vector(form, gravity, "gravity") })
	end
end

-- Returns a slide form: `form(ctx, entity, from, delta, options)`, with the call's result.
local function slideForm(form, name, vector, keys)
	return function(...)
		forms.arguments(form, 5, ...)
		local ctx, target, from, delta, options = ...
		checkContext(form, ctx)
		local fields = forms.options(form .. ": the options", options, keys)
		for _, key in keys do
			if fields[key] ~= nil then
				number(form .. ": the options", fields[key], key)
			end
		end
		local arguments = table.clone(fields)
		arguments.entity = entity(form, target)
		arguments.from = vector(form, from, "from")
		arguments.delta = vector(form, delta, "delta")
		return call(form, ctx, name, arguments)
	end
end

-- 2D: in world units, Y down. Needs the call of the same name (aylo.physics.<name>) in the system's `calls`.
engine.physics.raycast = rayForm("engine.physics.raycast", "aylo.physics.raycast", vec2)
engine.physics.shapeCast = castForm("engine.physics.shapeCast", "aylo.physics.shapeCast", vec2)
engine.physics.overlap = overlapForm("engine.physics.overlap", "aylo.physics.overlap", vec2)
engine.physics.moveAndSlide =
	slideForm("engine.physics.moveAndSlide", "aylo.physics.moveAndSlide", vec2, { "skin", "maxSlides" })
engine.physics.applyForce = pushForm("engine.physics.applyForce", "aylo.physics.applyForce", vec2)
engine.physics.applyImpulse = pushForm("engine.physics.applyImpulse", "aylo.physics.applyImpulse", vec2)
engine.physics.setGravity = gravityForm("engine.physics.setGravity", "aylo.physics.setGravity", vec2)

-- engine.physics.applyTorque(ctx, entity, torque): the call takes `torque`, not `value`.
function engine.physics.applyTorque(...)
	local form = "engine.physics.applyTorque"
	forms.arguments(form, 3, ...)
	local ctx, target, torque = ...
	checkContext(form, ctx)
	call(form, ctx, "aylo.physics.applyTorque", { entity = entity(form, target), torque = number(form, torque, "torque") })
end

-- 3D: in metres, Y up.
engine.physics.raycast3D = rayForm("engine.physics.raycast3D", "aylo.physics.raycast3D", vec3)
engine.physics.shapeCast3D = castForm("engine.physics.shapeCast3D", "aylo.physics.shapeCast3D", vec3, { "rotation" })
engine.physics.overlap3D = overlapForm("engine.physics.overlap3D", "aylo.physics.overlap3D", vec3, { "rotation" })
engine.physics.moveAndSlide3D = slideForm(
	"engine.physics.moveAndSlide3D",
	"aylo.physics.moveAndSlide3D",
	vec3,
	{ "skin", "maxSlides", "stepHeight", "maxSlope" }
)
engine.physics.applyForce3D = pushForm("engine.physics.applyForce3D", "aylo.physics.applyForce3D", vec3)
engine.physics.applyImpulse3D = pushForm("engine.physics.applyImpulse3D", "aylo.physics.applyImpulse3D", vec3)
engine.physics.applyTorque3D = pushForm("engine.physics.applyTorque3D", "aylo.physics.applyTorque3D", vec3)
engine.physics.setGravity3D = gravityForm("engine.physics.setGravity3D", "aylo.physics.setGravity3D", vec3)
