# Setup forms

Every game declares its assets and its input actions. The setup forms do it from one readable table each, and the same
script runs on the server and the clients: a side that does not have the feature (a server has no textures) skips the
loading and still gives you the asset's id, so the rest of the script does not care.

A form checks its whole table on every side. An unknown key, a value of the wrong type or a binding of no known shape is
an error that names the form and the part of it (`engine.assets.atlas: frame 2: 'size': missing 'y'`).

## `engine.assets`

Paths are relative to the game's `assets/` folder and cannot leave it. Each form returns the asset's id, to keep in a
table and use in components and calls.

```luau
Game.assets = {
	ship = engine.assets.atlas({
		name = "game.ship",
		texture = "spritesheet24.gif",
		grid = { frameSize = { x = 48, y = 48 }, count = 5, spacing = { x = 17, y = 0 }, offset = { x = 9, y = 9 } },
		animations = { fly = { frames = { 0, 1, 2, 3, 4 }, frameDuration = 0.1 } },
	}),
	font = engine.assets.font("fonts/orbitron.json"),
	music = engine.assets.sound("maint.ogg", { stream = true }),
	shot = engine.assets.sound("laser.mp3"),
	background = engine.assets.tilemap("levels/space-background.tmj"),
	planet = engine.assets.texture("backgrounds/space/parallax-space-big-planet.png"),
}
```

| Form | Returns | Notes |
|---|---|---|
| `engine.assets.texture(path, { linear, wrap })` | texture id | |
| `engine.assets.sound(path, { stream })` | sound id | `stream = true` decodes while playing, for music |
| `engine.assets.font(path)` | font id | a font atlas made with msdf-atlas-gen (`.json` next to its `.png`) |
| `engine.assets.tilemap(path)` | map id | a Tiled `.tmj` |
| `engine.assets.mesh(path)` | mesh id | a 3D model: glTF 2.0 (`.glb`, or `.gltf` with its files), FBX (`.fbx`) or Wavefront OBJ (`.obj` with its `.mtl`); needs `aylo.Mesh`, see [2D and 3D](3d.md#models-aylomeshrenderer) |
| `engine.assets.shader(path)` | shader id | written in the engine's GLSL dialect; `@aylo/bloom.glsl` and `@aylo/tonemap.glsl` are the engine's, see [Post-processing](post-processing.md) |
| `engine.assets.material({ name, shader, values, textures })` | material id | a shader and its values |
| `engine.assets.material(path)` | material id | a `.material` file, see [Materials](materials.md) |
| `engine.assets.atlas({ path \| texture, name, linear, grid, frames, animations })` | atlas id | see below |
| `engine.assets.particle({ name, ... })` | effect id | a particle effect, see [Particles](particles.md) |

### Atlases

An atlas names the frames of an image and the animations over them. Make it from a `path` to an atlas file (a
`.atlas` file, or an Aseprite or TexturePacker JSON export), or from a `texture` with a `grid` or a list of `frames`.

- `grid`: `frameSize` and `count` (both needed), `spacing`, `columns`, `offset`.
- `frames`: a list of `{ position, size, name?, origin? }`. A frame's name is its index when left out; its origin is its
  centre.
- `animations`: a table of animation name to `{ frames = {0, 1, 2} }` (consecutive frame indices, counted from 0) or
  `{ first, count }`, plus `frameDuration`, `direction` (for example `"pingPong"`) and `plays` (`0`: forever).
- `grid`, `frames` and `name` make an atlas from a `texture`, not from a `path`.

#### Atlas files

A `.atlas` file holds the same fields on disk, as JSON, so a [capsule](capsules.md#capsule-files) can name the atlas
without a script: `aylo.SpriteAnimator = { atlas = "ships/left.atlas", animation = "fly" }`. Write `texture` (an image)
with a `grid` or `frames`, or `path` (an Aseprite or TexturePacker export, whose animations it adds to), and optionally
`linear` and `animations`. Paths are relative to the `.atlas` file, and a vector is a list `[x, y]`. Unlike the form,
an animation's `frames` may list any frame indices, in any order.

```json
{
    "texture": "spritesheet24.gif",
    "grid": { "frameSize": [48, 48], "count": 5, "spacing": [17, 0], "offset": [9, 9] },
    "animations": { "fly": { "frames": [0, 1, 2, 3, 4], "frameDuration": 0.1 } }
}
```

`engine.assets.atlas({ path = "ships/left.atlas" })` reads it too, and gives the same id as a capsule naming it: the
file's path. A field the format does not have is an error (`Atlas 'ships/left.atlas': the atlas file: unknown field
'grdi' ...`), reported on the output; the game goes on without the atlas.

An atlas gives an entity its look through the `aylo.SpriteRenderer` component; an `aylo.SpriteAnimator` plays an
animation:

| Field | Type | Default | Meaning |
|---|---|---|---|
| `atlas` | atlas id | none | The atlas, from `engine.assets.atlas` |
| `animation` | string | none | The name of one of its animations; to switch, set it and put `time` back to `0` |
| `time` | number | `0` | Seconds since the animation started, at speed 1 |
| `speed` | number | `1` | Multiplies the time: `2` plays twice as fast, `0.5` half as fast, `0` freezes it |
| `playing` | boolean | `true` | `false`: the time stands still and the current frame stays |

```luau
ctx.commands:add(entity, "aylo.SpriteAnimator", { atlas = assets.cyclist, animation = "ride", speed = 1.5 })
```

An animation that plays a finite number of times stops on its last frame: `playing` turns `false`.

## `engine.input`

```luau
engine.input.action("move", "vector2", {
	bind = {
		{ up = "W", down = "S", left = "A", right = "D" },
		{ up = "Up", down = "Down", left = "Left", right = "Right" },
	},
})
engine.input.action("steer", "axis", { bind = { { negative = "Left", positive = "Right" } } })
engine.input.action("shoot", "button", { bind = { "MouseLeft", { control = "GamepadSouth", scale = 1 } } })
```

`engine.input.action(name, type, { bind })` declares an action. `type` is `"button"` (default), `"axis"` or
`"vector2"`. A binding is:

| Binding | Meaning |
|---|---|
| `"Space"` | a control by name |
| `{ control, scale }` | a control, scaled |
| `{ negative, positive }` | two controls making an axis |
| `{ up, down, left, right }` or `{ x, y }` | four controls or two stick axes making a vector |

The names of controls are in [Engine calls](engine-calls.md#input). In a networked game the action is also declared in
the network input layout on both sides, and the client sends its state to the server each tick. See
[Networking](networking.md).

## `engine.screenEffects`

```luau
local tonemap = engine.assets.material({ name = "game.tonemap", shader = engine.assets.shader("@aylo/tonemap.glsl") })
engine.screenEffects({ tonemap })
```

`engine.screenEffects(materials)` sets the screen effects, materials of `#pragma screen` shaders run in this order over
the whole picture; `{}` removes them. It does nothing on a server, so a script of both sides can call it. To change the
effects from a system, use the call `aylo.shader.setScreenMaterials`. See [Post-processing](post-processing.md).

## Forms you use in a system

Some forms work on the world, so they take what a system's `run` receives, its `ctx`, first. Use them rather than
`ctx.call`: they check their arguments, and their errors name the form and the argument.

```luau
engine.ecs.system("game.MoveShips", {
	phase = "FixedUpdate",
	query = { write = { "aylo.Transform2D" }, with = { "aylo.Collider", "net.PlayerInput" } },
	after = { "net.applyInputs", "aylo.Physics.SyncBodies" },
	run = function(ctx)
		for entity, transform in ctx.query do
			local move = engine.input.read(ctx, entity, "move").vector
			transform.position = engine.physics.moveAndSlide(ctx, entity, transform.position, { x = move.x * 4, y = -move.y * 4 })
		end
	end,
})
```

| Form | Result | Engine call underneath | Page |
|---|---|---|---|
| `engine.input.read(ctx, action)` | `{ value, held, pressed, released }`: the action on this machine (a client, single player) | `aylo.input.read` | [Engine calls](engine-calls.md#input) |
| `engine.input.read(ctx, entity, action)` | `{ held, pressed, released, axis, vector, clientTime }`: the action as the player who owns `entity` sent it (the server) | `aylo.replication.readInput` | [Networking](networking.md#inputs) |
| `engine.state.get(ctx, machine)`, `engine.state.set(ctx, machine, value)` | the value (`""` before the first frame); nothing | `aylo.state.get`, `aylo.state.set` | [States](states.md#changing-the-state) |
| `engine.audio.play(ctx, sound, { bus, volume, pitch, position, loop, startAt, lowpass })` | the voice id | `aylo.audio.playSound` | [Engine calls](engine-calls.md#audio) |
| `engine.net.localClient(ctx)` | `{ connected, client }` | `aylo.replication.localClient` | [Networking](networking.md#players-and-presence) |
| `engine.net.sendToServer(ctx, name, values)`, `engine.net.sendToClients(ctx, name, values, client)`, `engine.net.received(ctx)` | whether it was sent; the packets of the frame | `aylo.net.sendToServer`... | [Networking](networking.md#your-own-messages) |
| `engine.capsule.spawn(ctx, path, overrides)`, `engine.capsule.load(ctx, path, overrides)`, `engine.capsule.reload(ctx, root)` | the root | `aylo.capsule.spawn`... | [Capsules](capsules.md#spawning-switching-levels-reloading) |
| `engine.physics.raycast(ctx, ...)`, `moveAndSlide`, `shapeCast`, `overlap`... one per physics call, 2D and 3D | the hit, the position... | `aylo.physics.raycast`... | [Physics](physics.md#forms) |
| `engine.http.request(ctx, request)`, `engine.persistence.read(ctx, ...)`... | | `aylo.http.request`... | [HTTP](http.md), [Persistence](persistence.md) |
| `engine.ui.spawn(ctx.commands, spec, parent)` | the root | | [User interface](ui.md) |

Every form that takes `ctx` takes a [Behaviour's](behaviours.md) `self` in its place:
`engine.audio.play(self, sound)`, `engine.state.get(self, "game.Round")`. Anything else first is an error:
`engine.state.get: expected the system's ctx (or a Behaviour's self) first`.

A form makes an engine call, like `ctx.call`: the system does not need to list it (see
[What a system uses](systems.md#what-a-system-uses)). `ctx.call` stays what is underneath, for the calls that have no
form: see [Engine calls](engine-calls.md).
