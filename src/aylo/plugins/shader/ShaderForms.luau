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
	local spec = forms.table(form, (...), { "name", "shader", "values", "textures" })
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
	return material
end

-- engine.screenEffects({ material, ... }): the screen effects, materials of `#pragma screen` shaders run in this order
-- over the whole scene (aylo.shader.setScreenMaterials), replacing the previous ones; {} removes them. Where there is no
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
	forms.call(name, "aylo.shader.setScreenMaterials", { materials = materials })
end
