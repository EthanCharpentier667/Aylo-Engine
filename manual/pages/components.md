# Components

A component is a named bag of fields attached to an entity. You declare yours with `engine.ecs.component`, at load
time, in the same way on the server and on every client.

```luau
engine.ecs.component("game.Ship", {
	fields = { position = "vec2", player = "u32" },
	defaults = { player = 0 },
	replication = "everyChange",
})

engine.ecs.component("game.MisfireTimer", { fields = { remaining = "f32" }, presence = "serverOnly" })
```

## The declaration

| Option | Meaning |
|---|---|
| `fields` | **Required.** A table of field name to type name. An empty table makes a tag. |
| `defaults` | The starting value of some fields; a field not listed starts at zero. It can only name declared fields. |
| `replication` | `"none"` (default), `"everyChange"` (sent whenever it changes) or `"spawnOnly"` (sent once, when the entity appears). A replicated component needs at least one field: a tag cannot be sent, and the server stops at start naming it. |
| `presence` | `"both"` (default), `"serverOnly"` or `"clientOnly"`: which side the component exists on. |

Any other option is an error. Names are conventionally `game.Name`; the `aylo.` and `net.` prefixes belong to the
engine.

## Field types

| Type | In a script |
|---|---|
| `bool` | `true` / `false` |
| `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64` | a number |
| `f32`, `f64` | a number |
| `vec2`, `vec3`, `vec4` | a table `{ x = ..., y = ... }` (`z`, `w` for the larger ones) |
| `entity` | an entity |
| `assetId` | the id of an asset; a path is accepted where a value is written |
| `string` | a string (limited to 255 bytes) |

The layout of a component is the same on every machine. When a client connects, the server checks that both sides
declared the same components, and refuses a client that differs, naming the component.

## Engine components

The engine's own components are declared by its plugins and used by name, with the same views as yours:
`aylo.Name` (`value`, a text: what the editor shows an entity as, given by a [capsule](capsules.md) to each entity it
spawns), `aylo.Transform2D` and `aylo.Transform3D` (see [2D and 3D](3d.md)), `aylo.TransformInterpolation` (no fields: draws the entity smoothly between fixed ticks, see
[Smooth movement](systems.md#smooth-movement)), `aylo.Camera2D`, `aylo.SpriteRenderer`, `aylo.TextRenderer`, `aylo.ShapeRenderer`,
`aylo.AudioListener`, `aylo.Collider`, `aylo.Collider3D`, `aylo.RigidBody`, `aylo.Velocity`, `aylo.Velocity3D` (see
[Physics](physics.md)), and the network ones `net.Owner` and
`net.PlayerInput`. Fields are written the same way (`size = { x = 48, y = 48 }`). What exists depends on the plugins
the program and `project.luau` add: a server has no sprites.

### Enum fields

A field that picks one of a few values takes its name, a string, and a view reads the name back:

```luau
ctx.commands:spawn({
	["aylo.Transform2D"] = { position = { x = 400, y = 300 } },
	["aylo.Collider"] = { shape = "circle", radius = 36 },
	["aylo.RigidBody"] = { type = "kinematic" },
})

for _, collider in ctx.query do
	if collider.shape == "circle" then
		-- ...
	end
end
```

| Component | Field | Names (the first is the default, unless said) |
|---|---|---|
| `aylo.Collider` | `shape` | `"box"`, `"circle"`, `"capsule"` |
| `aylo.Collider3D` | `shape` | `"box"`, `"sphere"`, `"capsule"` |
| `aylo.RigidBody` | `type` | `"static"`, `"kinematic"`, `"dynamic"` (default `"dynamic"`) |
| `aylo.SpriteRenderer`, `aylo.ShapeRenderer`, `aylo.TextRenderer` | `blend` | `"alpha"`, `"additive"`, `"cutout"` (cutout: sprites only) |
| `aylo.ShapeRenderer` | `kind` | `"rect"`, `"circle"`, `"capsule"`, `"line"` |
| `aylo.TextRenderer`, `aylo.UiText` | `align` | `"left"`, `"center"`, `"right"` |
| `aylo.AudioSource` | `bus` | `"music"`, `"effects"` (default `"effects"`) |
| `aylo.ParticleEmitter` | `simulation` | `"auto"`, `"gpu"`, `"cpu"` |
| `aylo.Camera3D` | `projection` | `"perspective"`, `"orthographic"` (see [2D and 3D](3d.md#seeing-the-3d-world)) |

The layout fields of `aylo.UiNode` take the names of the [UI styles](ui.md#layout) (`"row-reverse"`,
`"space-between"`...).

A number is still accepted on write (`shape = 1` is a circle), but a script that compares a read value with a number
no longer matches: compare with the name. A name the field does not have is an error that lists the ones it takes:
`... 'round' is not one of box, circle, capsule`. [Capsule files](capsule-files.md) write the same names, quoted.

### Draw order

Sprites (`aylo.SpriteRenderer`), texts (`aylo.TextRenderer`) and particle emitters are drawn without a depth buffer:
what is drawn later covers what is drawn before. The engine draws them in this order, every frame the same:

1. **By `layer`**, lowest first: a higher layer is always on top.
2. **In one layer, by blend**: the cutout ones first (`blend = "cutout"`), then the alpha-blended ones
   (`blend = "alpha"`, the default), then the additive ones (`blend = "additive"`).
3. **Among cutout or alpha ones of the same layer, by entity**: the order does not change from one frame to the next,
   even when other entities are spawned or despawned. Additive ones need no order (adding light gives the same picture
   in any order), so the engine groups them to draw them faster.

```luau
ctx.commands:spawn({
	["aylo.Transform2D"] = { position = { x = 400, y = 300 } },
	["aylo.SpriteRenderer"] = { texture = "sprites/smoke.png", layer = 2 },  -- over every sprite of layer 1 and below
})
```

To put one sprite over another, give it a higher `layer`: within a layer, which of two overlapping sprites covers the
other follows the rules above, not the order you spawned them in.

## Reading and writing

A system receives a *view* of a component for each entity it matches (see [Systems](systems.md)). Fields read and write
like table fields: `ship.position`, `ship.player = 1`, `transform.position = { x = 4, y = 5 }`. A view is valid only
during the call to `run`: do not keep it. Writing a read-only view is an error. An entity is a value you can keep
(`entity.bits` identifies it, for example as a table key), and it stays valid across frames.

## Where components live

The server is authoritative. A component with `replication` other than `"none"` is sent from the server to the
clients, and **a system running on the client may not write it** (nor a `serverOnly` one); this is checked when the
system is declared and when a command is recorded. Put client-only presentation data in `presence = "clientOnly"`
components. See [Networking](networking.md).
