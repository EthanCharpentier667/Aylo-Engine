engine.assets.sound = engine.forms.file("engine.assets.sound", "aylo.audio.loadSound", { "stream" })

-- engine.audio.play(ctx, sound, { bus, volume, pitch, position, loop, startAt, lowpass }) -> the voice id: plays a
-- sound (its id, or its path) with aylo.audio.playSound. `ctx` is a system's, or a Behaviour's self.
engine.audio = {}

local playKeys = { "bus", "volume", "pitch", "position", "loop", "startAt", "lowpass" }

engine.audio.play = function(...)
	local form = "engine.audio.play"
	local forms = engine.forms
	forms.arguments(form, 3, ...)
	local ctx, sound, given = ...
	ctx = forms.context(form, ctx)
	if sound == nil or (type(sound) ~= "string" and typeof(sound) ~= "AssetId") then
		forms.fail(form, "expected the sound second: its id (engine.assets.sound) or its path")
	end
	local context = form .. ": options"
	local options = forms.options(context, given, playKeys)
	local fields = { sound = sound }
	fields.bus = forms.string(context, options, "bus")
	for _, key in { "volume", "pitch", "startAt", "lowpass" } do
		fields[key] = forms.number(context, options, key)
	end
	fields.position = forms.vec2(context, options, "position")
	fields.loop = forms.boolean(context, options, "loop")
	return forms.callIn(form, ctx, "aylo.audio.playSound", fields)
end
