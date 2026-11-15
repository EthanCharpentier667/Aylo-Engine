# Physics

The physics plugin gives your game bodies, colliders and collision events, in 2D and in [3D](#3d-bodies). Add it in
`project.luau`:

```luau
engine.settings.plugins = { "aylo.Physics", "aylo.PhysicsDebug" }
engine.settings.physics.gravity = { 0, 0 }            -- 2D: { x, y }, world units per second squared, Y down
engine.settings.physics.gravity3D = { 0, -9.81, 0 }   -- 3D: { x, y, z }, metres per second squared, Y up (the default)
```

The settings are in [Project settings](project-settings.md). The simulation runs on the server. Clients build the same
bodies so that questions (raycasts, `moveAndSlide`) answer against the same scene, but they never step it.

## Components

They are ordinary components you spawn with `ctx.commands` and query by name.

| Component | What it is |
|---|---|
| `aylo.Collider` | A shape, its material and its collision categories (fields below) |
| `aylo.RigidBody` | How it moves: static, kinematic or dynamic, its mass and damping (fields below) |
| `aylo.Velocity` | `linear` and `angular` velocity, in world units per second |
| `aylo.Transform2D` | The pose; write it to move a body |

An entity with a `Collider` and a `Transform` gets a body; without a `RigidBody`, it is a static body (a wall).

`aylo.Collider`, the shape and the material. Each shape reads its own fields: `size` for a box, `radius` for a circle,
`radius` and `halfHeight` for a capsule.

| Field | Type | Default | Meaning |
|---|---|---|---|
| `shape` | string | `"box"` | `"box"`, `"circle"` or `"capsule"` |
| `size` | vec2 | `{ x = 32, y = 32 }` | box: full width and height |
| `radius` | f32 | `16` | circle and capsule: the radius |
| `halfHeight` | f32 | `16` | capsule: half the length of its straight part, along Y |
| `offset` | vec2 | `{ x = 0, y = 0 }` | the shape's center relative to the `Transform` |
| `sensor` | bool | `false` | detects overlaps (trigger events) without colliding |
| `friction` | f32 | `0.2` | how much the surface resists sliding, usually 0 to 1 |
| `restitution` | f32 | `0` | how bouncy it is: 0 absorbs, 1 bounces back fully |
| `layer` | u32 | `1` | the [collision categories](#layers) it belongs to |
| `mask` | u32 | every category (`0x7FFF`) | the categories it collides with |

`aylo.RigidBody`, how the body moves:

| Field | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | `"dynamic"` | `"static"` (never moves), `"kinematic"` (moved by your script through its `Transform`, pushes dynamic bodies), `"dynamic"` (gravity, forces, collisions) |
| `mass` | f32 | `1` | in kilograms; only a dynamic body uses it |
| `linearDamping` | f32 | `0.05` | slows the velocity down over time, per second |
| `angularDamping` | f32 | `0.05` | slows the rotation down over time, per second |
| `gravityScale` | f32 | `1` | multiplies the world's gravity; `0` ignores it |
| `fixedRotation` | bool | `false` | never rotates |
| `continuousCollision` | bool | `false` | never tunnels through thin colliders when fast (bullets), at a cost |

Distances are in world units, Y down. `Collider` and `RigidBody` are replicated by the engine, so a client sees the server's bodies. A body's velocity and
position are set by writing its `aylo.Velocity` and `aylo.Transform2D`.

```luau
ctx.commands:spawn({
	["game.Laser"] = { position = muzzle, shooter = player },
	["aylo.Transform2D"] = { position = muzzle },
	["aylo.Collider"] = { size = { x = 60, y = 12 }, layer = 2, mask = 1 },
	["aylo.RigidBody"] = { gravityScale = 0, fixedRotation = true, continuousCollision = true },
	["aylo.Velocity"] = { linear = { x = 600, y = 0 } },
})
```

### Layers

A collider belongs to the categories in `layer` and collides with those in `mask`: 15 categories, bits 0 to 14. Two
colliders collide only when each one's layer is in the other's mask. Use `bit32.bor` to combine them.

## Events

Declare them under `events = { read = {...} }` and read them with `ctx.events.read`:

| Event | When |
|---|---|
| `aylo.CollisionStarted`, `aylo.CollisionEnded` | two bodies start or stop touching (`first`, `second` are entities). `CollisionStarted` also has where: `point` and `normal` (vec2) for two 2D bodies, `point3D` and `normal3D` (vec3) for two 3D bodies; the other pair stays zero |
| `aylo.TriggerEntered`, `aylo.TriggerExited` | a body enters or leaves a sensor |

## Calls

List the ones you use under `calls` and make them with `ctx.call`.

| Call | Arguments | Result |
|---|---|---|
| `aylo.physics.raycast` | `origin`, `direction`, `maxDistance`, `mask`, `ignore`, `includeSensors` | `{ hit, entity, point, normal, distance }` |
| `aylo.physics.shapeCast` | the shape fields of a `Collider`, `from`, `direction`, `maxDistance`, the filter | as `raycast` |
| `aylo.physics.overlap` | the shape fields, `at`, the filter | `{ count, first }` |
| `aylo.physics.moveAndSlide` | `entity` (its `Collider` is the shape, its mask the filter), `from`, `delta`, `skin`, `maxSlides` | the position it ends at |
| `aylo.physics.applyForce`, `applyImpulse` | `entity`, `value` | nothing |
| `aylo.physics.applyTorque` | `entity`, `torque` | nothing |
| `aylo.physics.setGravity` | `gravity` | nothing |
| `aylo.physics.raycast3D` | `origin`, `direction` (default `(0, 0, -1)`), `maxDistance`, `mask`, `ignore`, `includeSensors` | `{ hit, entity, point, normal, distance }`, in 3D |
| `aylo.physics.applyForce3D`, `applyImpulse3D`, `applyTorque3D` | `entity`, `value` (a vec3) | nothing |
| `aylo.physics.setGravity3D` | `gravity` (a vec3) | nothing |
| `aylo.physics.shapeCast3D` | the shape fields of a `Collider3D`, `from`, `rotation` (a quaternion, none by default), `direction` (default `(0, 0, -1)`), `maxDistance`, the filter | as `raycast3D`; `distance` is how far the shape travels before it touches, `0` if it starts inside a body |
| `aylo.physics.overlap3D` | the shape fields of a `Collider3D`, `at`, `rotation`, the filter | `{ count, first }` |
| `aylo.physics.moveAndSlide3D` | `entity` (its `Collider3D` is the shape, its mask the filter), `from`, `delta`, `skin` (`0.02`), `maxSlides` (`4`), `stepHeight` (`0`), `maxSlope` (`45`) | `{ position, grounded, groundNormal, ground }`, see [A character in 3D](#a-character-in-3d) |

A null `ignore` (the default) skips no entity.

## Ordering your systems

The physics runs in `FixedUpdate`. Order the systems that touch bodies around its steps:

- moving things by `aylo.Transform2D` or `aylo.Velocity`: `after = { "aylo.Physics.SyncBodies" }`,
  `before = { "aylo.Physics.SyncToPhysics" }`;
- reading positions the physics wrote: `after = { "aylo.Physics.SyncFromPhysics" }`;
- reacting to contacts: `after = { "aylo.Physics.Events" }`.

```luau
engine.ecs.system("game.Ships", {
	phase = "FixedUpdate",
	query = { write = { "aylo.Transform2D" }, with = { "game.Ship" } },
	calls = { "aylo.physics.moveAndSlide" },
	after = { "aylo.Physics.SyncBodies" },
	before = { "aylo.Physics.SyncToPhysics" },
	run = function(ctx)
		for ship, transform in ctx.query do
			transform.position = ctx.call("aylo.physics.moveAndSlide", {
				entity = ship, from = transform.position, delta = { x = 4, y = 0 },
			})
		end
	end,
})
```

## 3D bodies

A 3D entity gets a body the same way: an `aylo.Collider3D` and an `aylo.Transform3D`, and an `aylo.RigidBody` to make it
move. The 3D world is in metres, Y up (see [2D and 3D](3d.md)), and nothing is scaled by `unitsPerMeter`.

```luau
-- a floor, its top at y = 0, and a ball that falls on it and rests
ctx.commands:spawn({
	["aylo.Transform3D"] = { position = vector.create(0, -0.5, 0) },
	["aylo.Collider3D"] = { shape = "box", size = vector.create(10, 1, 10) },
})
ctx.commands:spawn({
	["aylo.Transform3D"] = { position = vector.create(0, 5, 0) },
	["aylo.Collider3D"] = { shape = "sphere", radius = 0.5 },
	["aylo.RigidBody"] = {},
})
```

| Component | What it is |
|---|---|
| `aylo.Collider3D` | A 3D shape, its material and its collision categories (fields below) |
| `aylo.RigidBody` | The same as in 2D; `fixedRotation` keeps a 3D body from turning around any axis |
| `aylo.Velocity3D` | `linear` (vec3, metres per second) and `angular` (vec3, radians per second around each world axis) |
| `aylo.Transform3D` | The pose; write it to move a body |

`aylo.Collider3D`. Each shape reads its own fields: `size` for a box, `radius` for a sphere, `radius` and `halfHeight` for
a capsule.

| Field | Type | Default | Meaning |
|---|---|---|---|
| `shape` | string | `"box"` | `"box"`, `"sphere"` or `"capsule"` |
| `size` | vec3 | `(1, 1, 1)` | box: full width, height and depth |
| `radius` | f32 | `0.5` | sphere and capsule: the radius |
| `halfHeight` | f32 | `0.5` | capsule: half the length of its straight part, along Y |
| `offset` | vec3 | `(0, 0, 0)` | the shape's center relative to the `Transform3D`, in its axes |
| `sensor`, `friction`, `restitution`, `layer`, `mask` | | | as for `aylo.Collider` |

The 3D bodies run through the same systems, with the same names (order your systems [as in 2D](#ordering-your-systems),
writing `aylo.Transform3D` and `aylo.Velocity3D`), and send the same events. `gravity3D` pulls them, `gravity` pulls
the 2D ones; `gravityScale` multiplies whichever applies.

```luau
-- a ball (radius 0.5) that hops whenever it has ground under it
engine.ecs.component("game.Hopper", { fields = {} })
engine.ecs.system("game.Hop", {
	phase = "FixedUpdate",
	query = { read = { "aylo.Transform3D" }, with = { "game.Hopper" } },
	calls = { "aylo.physics.raycast3D", "aylo.physics.applyImpulse3D" },
	after = { "aylo.Physics.SyncBodies" },
	before = { "aylo.Physics.SyncToPhysics" },
	run = function(ctx)
		for ball, transform in ctx.query do
			local ground = ctx.call("aylo.physics.raycast3D", {
				origin = transform.position, direction = vector.create(0, -1, 0), maxDistance = 0.55, ignore = ball,
			})
			if ground.hit then
				ctx.call("aylo.physics.applyImpulse3D", { entity = ball, value = vector.create(0, 4, 0) })
			end
		end
	end,
})
```

!!! warning "2D and 3D never touch"
    A 2D body and a 3D body never collide, even at the same place, and a 2D query (`raycast`, `overlap`...) never sees a
    3D body, nor `raycast3D` a 2D one. An entity has one body: give it a `Collider` or a `Collider3D`; with both, the
    `Collider3D` is used.

A 3D body is placed by its own `Transform3D`, even under a parent: a 3D body with a parent is placed as if it had none.

### Ask what a 3D shape would touch

`aylo.physics.shapeCast3D` sweeps a shape (a box, a sphere or a capsule, with the fields of a `Collider3D`) from a point
along a direction, and tells you the first body it would touch: a thick ray, for a melee swing or a fat bullet.
`aylo.physics.overlap3D` tells you which bodies a shape covers where it stands: an explosion, a trigger you test once.
Both take a `rotation` (a quaternion built with `engine.math.quat`, none by default), so a box can be turned.

```luau
-- the first body a 1 m box, swung 2 m forward from the player, would touch
local hit = ctx.call("aylo.physics.shapeCast3D", {
	shape = "box", size = vector.create(1, 1, 1),
	from = transform.position, rotation = transform.rotation,
	direction = vector.create(0, 0, -1), maxDistance = 2, ignore = entity,
})
-- how many bodies a sphere of 3 m around the blast covers
local blast = ctx.call("aylo.physics.overlap3D", { shape = "sphere", radius = 3, at = transform.position })
```

### A character in 3D

A first-person or third-person player is a capsule you move yourself: `aylo.physics.moveAndSlide3D` moves an entity's
`Collider3D` by a `delta` and handles what it meets.

- It stops a `skin` away from what it hits and **slides** along it, so the player runs along a wall instead of sticking.
- Ground no steeper than `maxSlope` (in degrees) is **walked on**, up and down. A steeper surface is a **wall**: the
  player slides along it but never climbs it, and slides down it if it lands there.
- With a `stepHeight` above `0`, it **steps up** what stops it if it is no higher than that (a stair, a kerb) and has
  ground on top.
- It tells you if the player is **`grounded`**, on what (`ground`, an entity) and the ground's `groundNormal`.

It keeps no speed: you add the gravity to the `delta` yourself, every tick. Give the player an
`aylo.RigidBody` of type `"kinematic"`, so its body follows the `Transform3D` you write and pushes the dynamic bodies it
walks into, and move it in `FixedUpdate` between `aylo.Physics.SyncBodies` and `aylo.Physics.SyncToPhysics`. Its own
body never blocks it.

```luau
-- Your input system writes wish: where the player wants to go, length 0 to 1, and jump.
engine.ecs.component("game.Player", { fields = { wish = "vec3", jump = "bool", fall = "f32", grounded = "bool" } })

local speed = 5 -- metres per second
local gravity = -20 -- metres per second squared
local jumpSpeed = 7 -- metres per second, up

engine.ecs.system("game.Walk", {
	phase = "FixedUpdate",
	query = { write = { "aylo.Transform3D", "game.Player" } },
	calls = { "aylo.physics.moveAndSlide3D" },
	after = { "aylo.Physics.SyncBodies" },
	before = { "aylo.Physics.SyncToPhysics" },
	run = function(ctx)
		local dt = ctx.fixedDelta
		for entity, transform, player in ctx.query do
			if player.grounded then
				player.fall = if player.jump then jumpSpeed else 0
			end
			player.fall += gravity * dt
			local moved = ctx.call("aylo.physics.moveAndSlide3D", {
				entity = entity,
				from = transform.position,
				delta = player.wish * (speed * dt) + vector.create(0, player.fall * dt, 0),
				stepHeight = 0.3,
			})
			transform.position = moved.position
			player.grounded = moved.grounded
		end
	end,
})
```

Spawn the player as a capsule standing on its centre:

```luau
ctx.commands:spawn({
	["game.Player"] = {},
	["aylo.Transform3D"] = { position = vector.create(0, 1, 0) },
	["aylo.Collider3D"] = { shape = "capsule", radius = 0.3, halfHeight = 0.6 }, -- 1.8 m tall
	["aylo.RigidBody"] = { type = "kinematic" },
})
```

| Argument | Default | Meaning |
|---|---|---|
| `entity` | | the entity that moves: its `Collider3D` is the shape (without rotation: a capsule stands along Y), its `mask` says what blocks it |
| `from` | `(0, 0, 0)` | where it starts: its `Transform3D` position |
| `delta` | `(0, 0, 0)` | how far it would move with nothing in the way, in metres, the gravity included |
| `skin` | `0.02` | the gap, in metres, kept between it and what it touches |
| `maxSlides` | `4` | how many surfaces one move slides along before it stops |
| `stepHeight` | `0` | the highest step it climbs, in metres; `0` climbs none |
| `maxSlope` | `45` | the steepest ground it walks on, in degrees, from `0` to below `90` |

| Result | Meaning |
|---|---|
| `position` | where it ends: write it to its `Transform3D` |
| `grounded` | whether it stands on walkable ground (within twice the `skin` below it) |
| `groundNormal` | that ground's normal, pointing up out of it; `(0, 0, 0)` when not grounded |
| `ground` | the entity it stands on (a moving platform you can follow); null when not grounded |

The call fails with `aylo.physics.moveAndSlide3D: entity <n> has no Collider3D` for an entity without one, and when
`skin` or `stepHeight` is negative or `maxSlope` is not from `0` to below `90`. Like everything physics does, it gives
the same result on the server and on a client that predicts its player from the same bodies (see
[Networking](networking.md)).

## The debug view

With `"aylo.PhysicsDebug"` in `plugins` (client only), F3 toggles outlines of the colliders and arrows for the velocities,
in 2D and in 3D (wireframes and arrows drawn in the 3D world, by a renderer that draws 3D).
`physicsDebug.drawAll = true` outlines every collider; otherwise only entities with an `aylo.PhysicsDebug` component are
drawn.
