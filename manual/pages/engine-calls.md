# Engine calls

Plain components cover most of a game. An engine call is for what data cannot express: loading an asset, binding an
input, playing a sound, asking the physics a question. It is a function the engine gives to scripts, with a name
`aylo.<feature>.<verb>`, called with a table of arguments.

```luau
-- while the script loads (top level of a file)
local sound = engine.call("aylo.audio.loadSound", { path = "laser.mp3" })

-- in a system: declare it, then call it
engine.ecs.system("game.Pew", {
	phase = "Update",
	side = "client",
	query = {},
	calls = { "aylo.audio.playSound" },
	run = function(ctx)
		ctx.call("aylo.audio.playSound", { sound = sound, volume = 0.9 })
	end,
})
```

- Arguments you leave out keep their defaults. An `assetId` argument accepts a path.
- A text argument or result is a Luau string. Short names (a sound, an action) hold 255 bytes at most and a longer
  one is cut; a call that takes or gives a long text (a prompt, a request, a reply) takes it whole, at any length,
  as its table says. A list argument is a Luau array.
- A result of one field is returned as that value, a result of several as a table, a call with no result returns
  nothing.
- A system that calls something it did not list under `calls` gets a Luau error. An unknown call is refused when the
  script loads, except in a system that does not run on that side (a client system may list a call that only the client
  has).
- An unknown name, a wrong argument or a failing call is a Luau error that names the call.
- A call exists only where its feature is: a server has no textures or sounds. The [setup forms](setup-forms.md) hide
  this for assets.

## The calls

### Sprites

An atlas is made with one call and filled by name with the others.

| Call | Arguments | Result |
|---|---|---|
| `aylo.sprite.loadAtlas` | `path` (an Aseprite or TexturePacker JSON) | the atlas id |
| `aylo.sprite.createAtlas` | `name`, `texture`, and for a grid `frameSize`, `count`, `spacing` | the atlas id |
| `aylo.sprite.addFrame` | `atlas`, `position`, `size` | the frame's index |
| `aylo.sprite.addAnimation` | `atlas`, `name`, `frameDuration`, `direction`, a range of frames | nothing |

Prefer the `engine.assets.atlas` form, which makes these calls from one table.

### Textures, fonts, maps

| Call | Arguments | Result |
|---|---|---|
| `aylo.backend.loadTexture` | `path` | the texture id |
| `aylo.text.loadFont` | `path` (a font atlas JSON) | the font id |
| `aylo.tilemap.load` | `path` (a Tiled `.tmj`) | the map id |
| `aylo.tilemap.spawn` | `map`, `position`, `scale`, `firstDrawLayer` | how many layers it spawned |

Texts are spawned as components:

```luau
ctx.commands:spawn({
	["aylo.Transform2D"] = { position = { x = 400, y = 12 } },
	["aylo.TextRenderer"] = { text = "Player 1", font = font, size = 22, anchor = { x = 0.5, y = 0 } },
})
```

### Audio

| Call | Arguments | Result |
|---|---|---|
| `aylo.audio.loadSound` | `path`, `stream` (decode while playing, for music) | the sound id |
| `aylo.audio.playSound` | `sound`, `bus` (`"effects"` or `"music"`, default `"effects"`), `volume` (1), `pitch` (1), `position`, `loop` (false), `startAt` (0, seconds), `lowpass` (0, Hz) | the voice id (a number, never 0) |
| `aylo.audio.getTime` | none | the audio clock, in seconds |
| `aylo.audio.setVolume` | `voice`, `volume` (linear gain, 1 = unchanged) | nothing |
| `aylo.audio.setFilter` | `voice`, `lowpass` (cutoff in Hz, 0 removes the filter) | nothing |
| `aylo.audio.stopVoice` | `voice`, `fadeOut` (seconds, 0 = at once) | nothing |

All audio calls run on the client only. A `position` left out plays the sound centred. An unknown `bus` raises
`Unknown bus 'x' (expected 'effects' or 'music')`. Music is started by sending the `aylo.PlayMusic` event
(`{ sound, crossfade }`), and stopped with `aylo.StopMusic`.

#### Scheduled playback

`playSound` returns a voice id. Keep it to change or stop that one sound later. `startAt` is a time on the clock
`aylo.audio.getTime` reads, so you schedule a sound relative to "now":

```luau
local kick = engine.call("aylo.audio.loadSound", { path = "kick.wav" })
local voice = 0

engine.ecs.system("game.Beat", {
	phase = "Update",
	side = "client",
	query = {},
	calls = { "aylo.audio.getTime", "aylo.audio.playSound", "aylo.audio.setVolume", "aylo.audio.setFilter" },
	run = function(ctx)
		if voice == 0 then
			local now = ctx.call("aylo.audio.getTime", {})
			voice = ctx.call("aylo.audio.playSound", { sound = kick, startAt = now + 0.5, loop = true })
		else
			ctx.call("aylo.audio.setVolume", { voice = voice, volume = 0.4 })
			ctx.call("aylo.audio.setFilter", { voice = voice, lowpass = 800 })
		end
	end,
})
```

To stop it, call `aylo.audio.stopVoice` (declare it in `calls`) with `{ voice = voice, fadeOut = 0.25 }`.

- Voices given the same `startAt` start on the same audio sample, so stems stay locked together.
- A `startAt` of 0, or a time already past, starts the voice at once.
- `loop = true` plays until you call `stopVoice`.
- The clock is the local audio device's, in seconds. It is 0 before the first frame. It is not the server's clock:
  mapping the two is up to the game.
- `setVolume`, `setFilter` and `stopVoice` on a voice that ended, was stopped, or whose sound was not loaded do
  nothing and raise no error.
- A sound starts at the end of the frame, so call `loadSound` early (at the top level of a file), not just before
  `playSound`.

### Input

Controls are named with text: a key without its prefix (`"W"`, `"Space"`, `"Up"`, `"LeftShift"`), `"Mouse"` plus a
button (`"MouseLeft"`), `"MouseDeltaX"`, `"MouseScrollY"`, `"Gamepad"` plus a button or axis (`"GamepadSouth"`,
`"GamepadLeftX"`). Keys are physical positions named after the US layout, so WASD lands on ZQSD on a French keyboard.

| Call | Arguments | Result |
|---|---|---|
| `aylo.input.addAction` | `name`, `type` (`"button"`, `"axis"`, `"vector2"`) | nothing |
| `aylo.input.bind` | `action`, `control`, optional `scale` | nothing |
| `aylo.input.bindAxis` | `action`, `negative`, `positive` | nothing |
| `aylo.input.bindVector` | `action`, `up`, `down`, `left`, `right` or `x`, `y` | nothing |
| `aylo.input.read` | `action` | `{ value, held, pressed, released }` |

Prefer `engine.input.action`, see [Setup forms](setup-forms.md). To read an action on the server, see
[Networking](networking.md).

### Physics

See [Physics](physics.md).

### Capsules

| Call | Arguments | Result |
|---|---|---|
| `aylo.capsule.spawn` | `capsule` (a path) | the root entity |
| `aylo.capsule.load` | `capsule` (a path): switches levels | the root entity |
| `aylo.capsule.reload` | `root` | `true` if it was a capsule's root |

See [Capsules](capsules.md).

### Replication

| Call | Where | What |
|---|---|---|
| `aylo.replication.addInput` | both | declares a network input (made for you by `engine.input.action`) |
| `aylo.replication.readInput` | server | the input of a player's action (`held`, `pressed`, `released`, `axis`, `vector`, and `clientTime`, untrusted), see [Networking](networking.md) |
| `aylo.net.serverTime` | both | `{ synced, time }`: the server clock, in seconds (estimated on a client), see [Server time and RTT](networking.md#server-time-and-rtt) |
| `aylo.net.rtt` | server | `{ client }` gives `{ connected, rtt }`: the smoothed round-trip time to that client, in seconds |
| `aylo.replication.localClient` | client | `{ connected, client }`: whether the server gave this client its id, and the id (as `net.Owner` and `net.ClientConnected` carry it) |

### HTTP

Server side, with `aylo.Http` added. Prefer `engine.http.request`, see [HTTP](http.md).

| Call | Arguments | Result |
|---|---|---|
| `aylo.http.request` | `url`, `method` (`"GET"`), `headers` (`Name: value` lines), `body`, `timeout` | the request's id; its response comes as `aylo.HttpResponse` |
| `aylo.http.cancel` | `request` | `true`, or `false` when it had already ended |

### States

On both sides, in every game. See [States](states.md).

| Call | Arguments | Result |
|---|---|---|
| `aylo.state.get` | `state` (a machine of `engine.ecs.state`) | its current value, `""` before its first frame |
| `aylo.state.set` | `state`, `value` | nothing; the machine changes at the next frame. On a client, an error for a machine declared with `replicate = true` |

### Shaders and particles

The shader plugin has `engine.assets.shader` and `engine.assets.material` forms, see [Setup forms](setup-forms.md),
and the call `aylo.shader.setScreenMaterials` (`materials`, client), which sets the screen effects: see
[Post-processing](post-processing.md), and its form `engine.screenEffects`.
Particle effects have the `engine.assets.particle` form, see [Particles](particles.md). It is made of the calls
`aylo.particle.beginEffect`, `aylo.particle.addBurst`, `aylo.particle.addColorKey`, `aylo.particle.addCurveKey` and
`aylo.particle.addEffect` (client); use the form rather than the calls.
