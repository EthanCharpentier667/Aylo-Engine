local forms = engine.forms
local form = "engine.assets.material"
local vectorKeys = { "x", "y", "z", "w" }

engine.assets.shader = forms.file("engine.assets.shader", "aylo.shader.load", {})

local function readValue(uniform, value)
	if type(value) == "number" then
		return { name = uniform, x = value }
	end
	local context = form .. ": value '" .. uniform .. "'"
	forms.table(context, value, vectorKeys)
	local fields = { name = uniform }
	local size = 0
	local given = 0
	for _ in value do
		given += 1
	end
	for _, key in vectorKeys do
		local component = forms.number(context, value, key)
		if component == nil then
			break
		end
		fields[key] = component
		size += 1
	end
	if size < 2 or size ~= given then
		forms.fail(context, "expected a number, or a vector {x, y}, {x, y, z} or {x, y, z, w}")
	end
	fields.size = size
	return fields
end

engine.assets.material = function(...)
	forms.arguments(form, 1, ...)
	if type((...)) == "string" then
		-- A material file: engine.assets.material("materials/hit.material").
		local path = (...)
		if not engine.hasCall("aylo.shader.loadMaterial") then
			return engine.assetId(path)
		end
		return forms.call(form, "aylo.shader.loadMaterial", { path = path })
	end
	local spec = forms.table(form, (...), { "name", "shader", "values", "textures", "renderState", "nextPass" })
	local name = forms.string(form, spec, "name")
	if name == nil or name == "" or spec.shader == nil then
		forms.fail(form, "expected a 'name' and a 'shader'")
	end
	local values = {}
	if spec.values ~= nil then
		for _, entry in forms.named(form .. ": 'values'", spec.values) do
			table.insert(values, readValue(entry[1], entry[2]))
		end
	end
	local textures = {}
	if spec.textures ~= nil then
		for _, entry in forms.named(form .. ": 'textures'", spec.textures) do
			table.insert(textures, { name = entry[1], texture = entry[2] })
		end
	end
	if not engine.hasCall("aylo.shader.addMaterial") then
		return engine.assetId(name)
	end
	local material = forms.call(form, "aylo.shader.addMaterial", { name = name, shader = spec.shader })
	for _, fields in values do
		fields.material = material
		forms.call(form, "aylo.shader.setMaterialValue", fields)
	end
	for _, fields in textures do
		fields.material = material
		forms.call(form, "aylo.shader.setMaterialTexture", fields)
	end
	if spec.renderState ~= nil then
		local context = form .. ": 'renderState'"
		local state = forms.table(context, spec.renderState, { "cull", "blend", "depth", "depthBias", "depthBiasSlope" })
		forms.call(form, "aylo.shader.setMaterialRenderState", {
			material = material,
			cull = forms.string(context, state, "cull"),
			blend = forms.string(context, state, "blend"),
			depth = forms.string(context, state, "depth"),
			depthBias = forms.number(context, state, "depthBias"),
			depthBiasSlope = forms.number(context, state, "depthBiasSlope"),
		})
	end
	if spec.nextPass ~= nil then
		forms.call(form, "aylo.shader.setMaterialNextPass", { material = material, nextPass = spec.nextPass })
	end
	return material
end

-- The entries of engine.screenEffects as passes: a material id is a pass of its own, a string the material of that shader
-- with its defaults, a table with a `material` a pass, and a list of passes (an engine.effects result) is spliced in.
local function passesOf(name, list)
	local passes = {}
	for _, entry in list do
		if type(entry) == "string" then
			local shader = engine.assets.shader(entry)
			table.insert(passes, { material = engine.assets.material({ name = "@aylo/effects/" .. entry, shader = shader }) })
		elseif type(entry) == "table" and entry.material == nil and type(entry[1]) == "table" then
			for _, pass in entry do
				table.insert(passes, pass)
			end
		elseif type(entry) == "table" then
			table.insert(passes, entry)
		else
			table.insert(passes, { material = entry })
		end
	end
	return passes
end

-- Whether every pass is only a material: the effects are then a plain chain (aylo.shader.setScreenMaterials).
local function isChain(passes)
	for _, pass in passes do
		for key in pass do
			if key ~= "material" then
				return false
			end
		end
	end
	return true
end

-- engine.screenEffects({ effect, ... }): the screen effects, in this order over the whole scene, replacing the previous
-- ones; {} removes them. An effect is a material of a `#pragma screen` shader, a shader's name ("@aylo/vignette.glsl"),
-- a pass { material, input, inputs, output, scale, size, lowPrecision } (aylo.shader.addScreenEffect), or what an
-- engine.effects function returns. Where there is no screen to draw (a server), it does nothing, so a script shared by
-- both sides can call it.
engine.screenEffects = function(...)
	local name = "engine.screenEffects"
	forms.arguments(name, 1, ...)
	local list = (...)
	if type(list) ~= "table" then
		forms.fail(name, "expected a list of materials")
	end
	if not engine.hasCall("aylo.shader.setScreenMaterials") then
		return
	end
	local passes = passesOf(name, list)
	if isChain(passes) then
		local materials = {}
		for _, pass in passes do
			table.insert(materials, pass.material)
		end
		forms.call(name, "aylo.shader.setScreenMaterials", { materials = materials })
		return
	end
	forms.call(name, "aylo.shader.setScreenMaterials", { materials = {} })
	for index, pass in passes do
		local context = name .. ": pass " .. index
		forms.table(context, pass, { "material", "input", "inputs", "output", "scale", "size", "lowPrecision" })
		if pass.material == nil then
			forms.fail(context, "expected a 'material'")
		end
		local size = forms.vec2(context, pass, "size")
		if size ~= nil and not (forms.isCount(size.x) and forms.isCount(size.y)) then
			forms.fail(context, "'size' must be whole numbers of pixels")
		end
		forms.call(name, "aylo.shader.addScreenEffect", {
			material = pass.material,
			input = forms.string(context, pass, "input"),
			output = forms.string(context, pass, "output"),
			scale = forms.number(context, pass, "scale"),
			width = size and size.x or nil,
			height = size and size.y or nil,
			lowPrecision = forms.boolean(context, pass, "lowPrecision"),
		})
		if pass.inputs ~= nil then
			for _, entry in forms.named(context .. ": 'inputs'", pass.inputs) do
				if type(entry[2]) ~= "string" then
					forms.fail(context, "'inputs." .. entry[1] .. "' must be the name of a target")
				end
				forms.call(name, "aylo.shader.addScreenEffectInput", { sampler = entry[1], target = entry[2] })
			end
		end
	end
	forms.call(name, "aylo.shader.checkScreenEffects", {})
end

-- engine.effects: the engine's standard screen effects. Each function takes the effect's values (all optional) and
-- returns its passes, for engine.screenEffects: engine.screenEffects({ engine.effects.bloom({ threshold = 0.8 }) }).
-- Each call makes materials and targets of its own, so an effect may appear twice with different values.
engine.effects = {}
local effectCount = 0

-- A new material of the engine's shader `@aylo/<shader>.glsl` with `values`.
local function effectMaterial(effect, shader, values)
	effectCount += 1
	return engine.assets.material({
		name = "@aylo/effects/" .. effect .. "/" .. effectCount,
		shader = engine.assets.shader("@aylo/" .. shader .. ".glsl"),
		values = values,
	})
end

-- The values given to engine.effects.<effect>, checked against `keys`.
local function effectValues(effect, keys, ...)
	local context = "engine.effects." .. effect
	forms.arguments(context, 1, ...)
	local values = (...)
	if values == nil then
		return {}, context
	end
	return forms.table(context, values, keys), context
end

-- A prefix for the targets of one call, so two calls do not share them.
local function targetPrefix(effect)
	effectCount += 1
	return "@aylo/" .. effect .. effectCount .. "/"
end

-- The effects of one pass: their values are their shader's.
local singleEffects = {
	sharpen = { shader = "sharpen", keys = { "amount", "mode" } },
	chromaticAberration = { shader = "chromatic_aberration", keys = { "strength", "offset" } },
	filmGrain = { shader = "film_grain", keys = { "luminanceNoise", "chromaNoise", "size", "speed" } },
	vignette = { shader = "vignette", keys = { "intensity", "radius", "softness", "color" } },
	fxaa = { shader = "fxaa", keys = { "spanMax", "reduceMul" } },
	fog = { shader = "fog", keys = { "color", "density", "start", "height", "heightFalloff" } },
	tonemap = { shader = "tonemap", keys = { "exposure", "mode" } },
}
for effect, spec in singleEffects do
	engine.effects[effect] = function(...)
		local values = effectValues(effect, spec.keys, ...)
		return { { material = effectMaterial(effect, spec.shader, values) } }
	end
end

-- A bloom on a chain of smaller and smaller images, down then back up: wide glows for little cost. Its values are
-- those of @aylo/bloom.glsl; `radius` (pixels) chooses how many halvings it makes.
engine.effects.bloom = function(...)
	local values, context = effectValues("bloom", { "threshold", "softKnee", "intensity", "radius" }, ...)
	local radius = forms.number(context, values, "radius") or 24
	local levels = math.clamp(math.ceil(math.log(math.max(radius, 2) / 2, 2)), 1, 8)
	local prefix = targetPrefix("bloom")
	local prefilter =
		effectMaterial("bloom", "bloom_prefilter", { threshold = values.threshold, softKnee = values.softKnee })
	local down = effectMaterial("bloom", "bloom_down")
	local up = effectMaterial("bloom", "bloom_up")
	local composite = effectMaterial("bloom", "bloom_composite", { intensity = values.intensity, levels = levels })
	local passes = { { material = prefilter, output = prefix .. "0", scale = 0.5 } }
	for level = 1, levels - 1 do
		table.insert(passes, {
			material = down,
			input = prefix .. (level - 1),
			output = prefix .. level,
			scale = 0.5 ^ (level + 1),
		})
	end
	local current = prefix .. (levels - 1)
	for level = levels - 2, 0, -1 do
		local output = prefix .. "up" .. level
		table.insert(passes, {
			material = up,
			input = current,
			inputs = { level = prefix .. level },
			output = output,
			scale = 0.5 ^ (level + 1),
		})
		current = output
	end
	table.insert(passes, { material = composite, inputs = { bloom = current } })
	return passes
end

-- The Kuwahara filter, a painterly look: the classic one in one pass, or with `anisotropic = true` the one that follows
-- the picture's strokes (four passes).
engine.effects.kuwahara = function(...)
	local values, context = effectValues(
		"kuwahara",
		{ "radius", "anisotropic", "sharpness", "eccentricity", "uniformity" },
		...
	)
	if not forms.boolean(context, values, "anisotropic") then
		return { { material = effectMaterial("kuwahara", "kuwahara", { radius = values.radius }) } }
	end
	local prefix = targetPrefix("kuwahara")
	local sigma = forms.number(context, values, "uniformity") or 2
	local tensor = effectMaterial("kuwahara", "kuwahara_tensor")
	local across = effectMaterial("kuwahara", "blur", { direction = { x = 1, y = 0 }, sigma = sigma })
	local down = effectMaterial("kuwahara", "blur", { direction = { x = 0, y = 1 }, sigma = sigma })
	local filter = effectMaterial("kuwahara", "kuwahara_anisotropic", {
		radius = values.radius,
		sharpness = values.sharpness,
		eccentricity = values.eccentricity,
	})
	return {
		{ material = tensor, output = prefix .. "tensor" },
		{ material = across, input = prefix .. "tensor", output = prefix .. "across" },
		{ material = down, input = prefix .. "across", output = prefix .. "smooth" },
		{ material = filter, inputs = { tensor = prefix .. "smooth" } },
	}
end

-- Screen-space ambient occlusion: the 3D world darkened in its creases and corners, at half size, blurred, then laid
-- over the picture (four passes).
engine.effects.ssao = function(...)
	local values, context =
		effectValues("ssao", { "radius", "intensity", "bias", "samples", "blur", "shadow" }, ...)
	local prefix = targetPrefix("ssao")
	local sigma = forms.number(context, values, "blur") or 2
	local occlusion = effectMaterial("ssao", "ssao", {
		radius = values.radius,
		intensity = values.intensity,
		bias = values.bias,
		samples = values.samples,
	})
	local across = effectMaterial("ssao", "blur", { direction = { x = 1, y = 0 }, sigma = sigma })
	local down = effectMaterial("ssao", "blur", { direction = { x = 0, y = 1 }, sigma = sigma })
	local apply = effectMaterial("ssao", "ssao_apply", { shadow = values.shadow })
	return {
		{ material = occlusion, output = prefix .. "occlusion", scale = 0.5, lowPrecision = true },
		{ material = across, input = prefix .. "occlusion", output = prefix .. "across", scale = 0.5, lowPrecision = true },
		{ material = down, input = prefix .. "across", output = prefix .. "smooth", scale = 0.5, lowPrecision = true },
		{ material = apply, inputs = { occlusion = prefix .. "smooth" } },
	}
end
