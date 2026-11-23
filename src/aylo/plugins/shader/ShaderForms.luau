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

-- engine.screenEffects({ material, ... }): the screen effects, materials of `#pragma screen` shaders run in this order
-- over the whole scene (aylo.shader.setScreenMaterials), replacing the previous ones; {} removes them. A list of tables
-- { material, input, inputs, output, scale, size, lowPrecision } is passes (aylo.shader.addScreenEffect). Where there is no
-- screen to draw (a server), it does nothing, so a script shared by both sides can call it.
engine.screenEffects = function(...)
	local name = "engine.screenEffects"
	forms.arguments(name, 1, ...)
	local materials = (...)
	if type(materials) ~= "table" then
		forms.fail(name, "expected a list of materials")
	end
	if not engine.hasCall("aylo.shader.setScreenMaterials") then
		return
	end
	-- A list of materials is a chain at full size; a list of tables is passes, each written as one call.
	if #materials == 0 or type(materials[1]) ~= "table" then
		forms.call(name, "aylo.shader.setScreenMaterials", { materials = materials })
		return
	end
	forms.call(name, "aylo.shader.setScreenMaterials", { materials = {} })
	for index, pass in materials do
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
