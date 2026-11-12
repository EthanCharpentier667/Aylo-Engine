# Physics

The physics plugin gives your game 2D bodies, colliders and collision events. Add it in `project.luau`:

```luau
engine.settings.plugins = { "aylo.Physics", "aylo.PhysicsDebug" }
engine.settings.physics.gravity = { 0, 0 }     -- { x, y }, world units per second squared, Y down
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
| `aylo.Transform` | The pose; write it to move a body |

An entity with a `Collider` and a `Transform` gets a body; without a `RigidBody`, it is a static body (a wall).

`aylo.Collider`, the shape and the material. Each shape reads its own fields: `size` for a box, `radius` for a circle,
`radius` and `halfHeight` for a capsule.

| Field | Type | Default | Meaning |
|---|---|---|---|
| `shape` | u8 | `0` | `0` box, `1` circle, `2` capsule |
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
| `type` | u8 | `2` | `0` static (never moves), `1` kinematic (moved by your script through its `Transform`, pushes dynamic bodies), `2` dynamic (gravity, forces, collisions) |
| `mass` | f32 | `1` | in kilograms; only a dynamic body uses it |
| `linearDamping` | f32 | `0.05` | slows the velocity down over time, per second |
| `angularDamping` | f32 | `0.05` | slows the rotation down over time, per second |
| `gravityScale` | f32 | `1` | multiplies the world's gravity; `0` ignores it |
| `fixedRotation` | bool | `false` | never rotates |
| `continuousCollision` | bool | `false` | never tunnels through thin colliders when fast (bullets), at a cost |

Distances are in world units, Y down. `Collider` and `RigidBody` are replicated by the engine, so a client sees the server's bodies. A body's velocity and
position are set by writing its `aylo.Velocity` and `aylo.Transform`.

```luau
ctx.commands:spawn({
	["game.Laser"] = { position = muzzle, shooter = player },
	["aylo.Transform"] = { position = muzzle },
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
| `aylo.CollisionStarted`, `aylo.CollisionEnded` | two bodies start or stop touching (`first`, `second` are entities) |
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

A null `ignore` (the default) skips no entity.

## Ordering your systems

The physics runs in `FixedUpdate`. Order the systems that touch bodies around its steps:

- moving things by `aylo.Transform` or `aylo.Velocity`: `after = { "aylo.Physics.SyncBodies" }`,
  `before = { "aylo.Physics.SyncToPhysics" }`;
- reading positions the physics wrote: `after = { "aylo.Physics.SyncFromPhysics" }`;
- reacting to contacts: `after = { "aylo.Physics.Events" }`.

```luau
engine.ecs.system("game.Ships", {
	phase = "FixedUpdate",
	query = { write = { "aylo.Transform" }, with = { "game.Ship" } },
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

## The debug view

With `"aylo.PhysicsDebug"` in `plugins` (client only), F3 toggles outlines of the colliders and arrows for the velocities.
`physicsDebug.drawAll = true` outlines every collider; otherwise only entities with an `aylo.PhysicsDebug` component are
drawn.
