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
| `aylo.audio.playSound` | `sound`, `bus` (`"effects"` or `"music"`, default `"effects"`), `volume` (1), `pitch` (1), `position` (a 2D table `{ x, y }`, in world units), `loop` (false), `startAt` (0, seconds), `lowpass` (0, Hz) | the voice id (a number, never 0) |
| `aylo.audio.playSound3D` | `sound`, `bus`, `volume`, `pitch`, `position` (a 3D point, in metres), `minDistance` (1), `maxDistance` (50), `loop`, `startAt`, `lowpass`, the defaults of `playSound` | the voice id |
| `aylo.audio.setListener3D` | `position` (a 3D point), `rotation` (a quaternion, none by default) | nothing |
| `aylo.audio.getTime` | none | the audio clock, in seconds |
| `aylo.audio.getOutputLatency` | none | how long a sound takes from that clock to the speaker, in seconds (`0` on the `"null"` backend and on a server) |
| `aylo.audio.setVolume` | `voice`, `volume` (linear gain, 1 = unchanged) | nothing |
| `aylo.audio.setFilter` | `voice`, `lowpass` (cutoff in Hz, 0 removes the filter) | nothing |
| `aylo.audio.stopVoice` | `voice`, `fadeOut` (seconds, 0 = at once) | nothing |

All audio calls run on the client only. `playSound`'s `position` is a 2D point `{ x, y }` in world units, panned left
or right from the `aylo.AudioListener` entity's `Transform2D` (see `audio.panDistance` in
[Project settings](project-settings.md)); left out, the sound plays centred. For a 3D game, use `playSound3D` (below). An unknown `bus` raises
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
- A sound scheduled at a time on that clock is **heard a little later**: the audio device fills a buffer before the
  speaker plays it, often 10 to 40 ms depending on the system. `aylo.audio.getOutputLatency` gives that delay, in
  seconds. To have a sound heard at a moment you know on another clock (a beat on the screen, the server's time),
  schedule it that much earlier:

  ```luau
  local music = engine.call("aylo.audio.loadSound", { path = "music/track.ogg", stream = true })
  local started = false

  engine.ecs.system("game.StartMusic", {
  	phase = "Update",
  	side = "client",
  	query = {},
  	calls = { "aylo.audio.getTime", "aylo.audio.getOutputLatency", "aylo.audio.playSound" },
  	run = function(ctx)
  		if started then
  			return
  		end
  		started = true
  		local due = 2 -- heard 2 seconds from now
  		local now = ctx.call("aylo.audio.getTime", {})
  		local latency = ctx.call("aylo.audio.getOutputLatency", {})
  		ctx.call("aylo.audio.playSound", { sound = music, bus = "music", startAt = now + due - latency })
  	end,
  })
  ```

  It is an estimate of the device's buffer, not exact to the sample: a constant offset is what it corrects. It can
  change when the system switches to another output (headphones plugged in), so read it each time you schedule,
  not once at start. It is `0` where nothing is heard (the `"null"` backend, a server).
- `setVolume`, `setFilter` and `stopVoice` on a voice that ended, was stopped, or whose sound was not loaded do
  nothing and raise no error.
- A sound starts at the end of the frame, so call `loadSound` early (at the top level of a file), not just before
  `playSound`.

#### Sound in 3D

In a 3D game, a sound plays from a place in the world: `aylo.audio.playSound3D` takes a 3D `position` (a Luau `vector`
or a table `{ x, y, z }`, in metres, Y up). The player hears it from the **3D listener**: on the left or the right, and
quieter the further it is.

The 3D listener is your camera. Give the camera entity an `aylo.AudioListener` next to its `aylo.Transform3D`, and it
hears from where it is and where it faces, with nothing else to do:

```luau
ctx.commands:spawn({
	["aylo.Transform3D"] = { position = vector.create(0, 1.7, 0) },
	["aylo.Camera3D"] = {},
	["aylo.AudioListener"] = {},
})
```

Or set it yourself, every frame, with `aylo.audio.setListener3D`. Once called, it wins over the entity: use one or the
other.

```luau
local hit = engine.call("aylo.audio.loadSound", { path = "sounds/hit.wav" })

engine.ecs.system("game.Ears", {
	phase = "Update",
	side = "client",
	query = { read = { "aylo.Transform3D" }, with = { "aylo.Camera3D" } },
	calls = { "aylo.audio.setListener3D", "aylo.audio.playSound3D" },
	run = function(ctx)
		for _, eye in ctx.query do
			ctx.call("aylo.audio.setListener3D", { position = eye.position, rotation = eye.rotation })
		end
		-- a hit 4 m to the right of the world's origin, heard from the camera
		ctx.call("aylo.audio.playSound3D", {
			sound = hit, position = vector.create(4, 0, 0), minDistance = 2, maxDistance = 40,
		})
	end,
})
```

- **Pan.** A sound on the listener's right plays in the right ear, on its left in the left ear, in front of or behind
  it in both. The listener looks along -Z with +X on its right, as a camera.
- **Distance.** Closer than `minDistance`, the sound plays at its `volume`. Further, its volume is divided by the
  distance: twice `minDistance` away, it is at half; four times, a quarter. It stops getting quieter at `maxDistance`.
  With `minDistance = 2` and `maxDistance = 40`, a sound 40 m away (or more) plays at 5% of its volume.
- **It follows the listener.** While the sound plays, its pan and volume are worked out again every frame, so it stays
  at its place in the world when the player turns or walks. The sound itself does not move.
- **Without a 3D listener** (no call, and no `aylo.AudioListener` with a `Transform3D`), a 3D sound plays centred at its
  full volume. A 2D `aylo.AudioListener` (with a `Transform2D`) only hears `playSound`.
- `setVolume`, `setFilter` and `stopVoice` work on its voice id as on `playSound`'s; `setVolume` sets its volume before
  the distance.
- `playSound3D` raises `aylo.audio.playSound3D: the distances must be 0 < minDistance <= maxDistance` for distances that
  cannot be.

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

#### Capture the cursor (mouse-look)

A first-person or third-person camera turns with the mouse. For that the cursor must not leave the window: you
*capture* it. A captured cursor is hidden, stays in the window, and `"MouseDeltaX"` / `"MouseDeltaY"` keep reporting the
movement however far the mouse goes.

Capture it from the start in `project.luau`:

```luau
engine.settings.window.captureCursor = true
```

Or capture and release it while the game runs, for a pause menu:

| Call | Arguments | Result |
|---|---|---|
| `aylo.graphics.captureCursor` | `capture` (`true` by default: capture; `false`: give the cursor back) | nothing |

The change takes effect at the start of the next frame. The `aylo.CursorCapture` resource tells you where it stands
(client side; read it with `resources = { read = { "aylo.CursorCapture" } }`):

| Field | Type | Meaning |
|---|---|---|
| `wanted` | `bool` | What your game asked for: the setting, then the last `captureCursor` call |
| `focused` | `bool` | Whether the window has the focus |
| `captured` | `bool` | Whether the cursor is captured now |

The engine gives the cursor back on its own when the player switches to another window, and captures it again when
they come back, as long as `wanted` is `true`.

In the [editor](editor.md), the cursor is captured only in the **Game** panel: while the game plays (Play, not paused)
and the Game panel has the focus. Anywhere else (editing, paused, another panel focused) `captured` stays `false`, so
the editor keeps its cursor. While the game holds it, **Escape** always gives it back, whatever your game does with the
key; a click on the game captures it again.

The engine does not decide which key frees the cursor: many games use Escape for a menu. This recipe toggles the
capture with Escape and turns a camera with the mouse:

```luau
engine.input.action("look", "vector2", { bind = { { x = "MouseDeltaX", y = "MouseDeltaY" } } })
engine.input.action("menu", "button", { bind = { "Escape" } })

local yaw, pitch = 0, 0
local sensitivity = 0.003 -- radians per pixel

engine.ecs.system("game.MouseLook", {
	phase = "Update",
	side = "client",
	resources = { read = { "aylo.CursorCapture" } },
	calls = { "aylo.input.read", "aylo.graphics.captureCursor" },
	run = function(ctx)
		local cursor = ctx.resources["aylo.CursorCapture"]
		if ctx.call("aylo.input.read", { action = "menu" }).pressed then
			ctx.call("aylo.graphics.captureCursor", { capture = not cursor.wanted })
		end
		if not cursor.captured then
			return -- the menu is open, or the window lost the focus: the mouse is the player's
		end
		local look = ctx.call("aylo.input.read", { action = "look" }).value
		yaw -= look.x * sensitivity
		pitch = math.clamp(pitch + look.y * sensitivity, -1.5, 1.5)
		-- turn your camera's aylo.Transform3D with yaw and pitch here
	end,
})
```

`"MouseDeltaX"` is the movement of the frame in pixels, to the right; `"MouseDeltaY"` is up positive. A server has the
call too, and it does nothing there: there is no window.

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
