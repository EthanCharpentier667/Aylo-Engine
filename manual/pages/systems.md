# Systems

A system is a function that runs on a schedule, on the entities that match a query. You declare one with
`engine.ecs.system`.

```luau
engine.ecs.system("game.MoveShips", {
	phase = "FixedUpdate",
	side = "server",
	query = { write = { "game.Ship", "aylo.Transform" }, with = { "net.PlayerInput" } },
	calls = { "aylo.replication.readInput" },
	after = { "net.applyInputs" },
	run = function(ctx)
		for entity, ship, transform in ctx.query do
			local move = ctx.call("aylo.replication.readInput", { entity = entity, action = "move" }).vector
			transform.position = { x = transform.position.x + move.x * 4, y = transform.position.y - move.y * 4 }
		end
	end,
})
```

## The declaration

| Option | Meaning |
|---|---|
| `phase` | **Required** (except with `onEnter` or `onExit`). When it runs, see below. |
| `query` | Which entities it visits, see [Queries](#queries). Leave it out (or write `{}`) for a system that visits none. |
| `run` | **Required.** The function, called with `ctx`. |
| `side` | `"server"` (default), `"client"` or `"both"`: where it runs. |
| `resources` | `{ read = {...}, write = {...} }`: the resources it uses. |
| `events` | `{ read = {...}, send = {...} }`: the events it reads or sends. |
| `calls` | The [engine calls](engine-calls.md) it makes with `ctx.call`. |
| `after`, `before` | Names of systems it must run after or before. |
| `inState`, `onEnter`, `onExit` | `{ machine, value }`: runs only in a game state, or once when the game enters or leaves it. See [States](states.md). |

Any other option is an error naming the system. A system must declare everything it touches: using an undeclared
resource, event or call is an error.

## Phases

A frame runs its phases in this order.

| Phase | Use |
|---|---|
| `Startup` | Once, before the first frame: setup, initial entities. |
| `PreUpdate` | Polling the window and input, receiving network packets. |
| `StateTransition` | Only when a [state](states.md) changes: its `onExit`, then its `onEnter` systems. Never written as a `phase`. |
| `FixedUpdate` | Gameplay at a fixed rate; also the physics and the server's network send. |
| `Update` | Once per frame: animation, camera, UI. |
| `PostUpdate` | Transform propagation, sending input (client). |
| `Render` | Extracting and drawing. |

Gameplay on the server goes in `FixedUpdate`; what a client shows goes in `Update`. Use `after` and `before` to order
systems of the same phase, including the engine's (the physics names are in [Physics](physics.md)).

## Smooth movement

`FixedUpdate` runs 60 times a second, whatever the screen does. On a 144 Hz screen, some frames see no tick and others
see one, so an entity moved in `FixedUpdate` stops and jumps: it judders. Add `aylo.TransformInterpolation` next to
its `aylo.Transform`, and the engine draws it between its last two ticks instead:

```luau
engine.ecs.system("game.SpawnBall", {
	phase = "Startup",
	side = "server",
	run = function(ctx)
		ctx.commands:spawn({
			["aylo.Transform"] = { position = { x = 400, y = 300 } },
			["aylo.TransformInterpolation"] = {},
		})
	end,
})
```

The component has no fields. On the client (and in single player and the editor), after the ticks of each frame, the
engine records the entity's `aylo.Transform` and computes the position you see, `aylo.GlobalTransform`, from the last
two records, blended by how far the frame is into the next tick. Its children follow the blended position.

- **What is drawn is up to one tick behind** the simulation (about 17 ms at 60 ticks a second). `aylo.Transform`, the
  physics and the server keep the simulated position: read `aylo.Transform` for gameplay.
- **A move made out of the ticks is drawn at once**: a teleport written in `Update`, or an entity moved in the editor,
  does not slide there.
- **Add it only to what moves in `FixedUpdate`** (your gameplay, or a body of the [physics](physics.md)). An entity a
  client moves in `Update`, such as one following replicated data, gains nothing from it.
- The server never blends: a dedicated server draws nothing, so its `aylo.GlobalTransform` is the `aylo.Transform`.

## Queries

A system without a `query` visits no entity: it runs once each time its phase runs (`Startup` once, `FixedUpdate` on
every tick, `Update` every frame), and `ctx.query` ends at once. Use it to set the game up, spawn entities, or read
resources and events:

```luau
engine.ecs.system("game.SpawnShip", {
	phase = "Startup",
	run = function(ctx)
		ctx.commands:spawn({ ["game.Shield"] = { strength = 10, regen = 5 } })
	end,
})
```

A system that visits entities lists what it asks of them:

```luau
query = {
	read = { "aylo.Transform" },
	write = { "game.Ship" },
	optionalRead = {}, optionalWrite = {},
	with = { "net.Owner" }, without = { "game.Dead" },
	added = { "game.Laser" }, changed = { "game.Ship" },
}
```

| Term | Meaning |
|---|---|
| `read`, `write` | The entity must have the component, and the loop gets a view of it (read-only, or writable). |
| `optionalRead`, `optionalWrite` | A view, or `nil` when the entity does not have it. |
| `with`, `without` | The entity must have, or must not have, the component. No view. |
| `added` | The entity must have it, and it was added since this system last ran. |
| `changed` | The entity must have it, and it was added or written since this system last ran. |

`for entity, a, b in ctx.query do` gives the entity, then one view per `read`/`write` term in the order read, write,
optionalRead, optionalWrite (inside each, in the order you listed them). The views are valid only during the call to
`run`. An unknown term name is an error.

## `ctx`

| Field | What it gives |
|---|---|
| `ctx.query` | The loop over matching entities. |
| `ctx.side` | The side the system runs on: `"server"` (the default), `"client"` or `"both"`. |
| `ctx.time` | `delta` (seconds of the frame), `fixedDelta` (the fixed step), `elapsed`, `tick` and `alpha` (progress between two fixed steps). `ctx.delta` and `ctx.fixedDelta` also exist. |
| `ctx.commands` | Deferred changes to the world, see below. |
| `ctx.resources` | The resources the system declared: `ctx.resources["aylo.Input"]` is a view (read-only unless declared in `write`). |
| `ctx.events` | `ctx.events.read(name)` loops over the events not read yet; `ctx.events.send(name, fields)` sends one. Both need the name in `events`. |
| `ctx.call(name, args)` | Makes an engine call the system listed in `calls`. |

### Commands

Changes to the world are recorded and applied after the system, not during the loop (a spawned entity cannot be queried by the loop that spawned it):

```luau
local ship = ctx.commands:spawn({
	["game.Ship"] = { position = position, player = 0 },
	["aylo.Transform"] = { position = position },
})
ctx.commands:add(ship, "aylo.RigidBody", { type = "kinematic" })
ctx.commands:patch(ship, "game.Ship", { player = 1 })
ctx.commands:setParent(child, parent)
ctx.commands:removeParent(child)
ctx.commands:despawn(ship)
```

`spawn` takes a table of component name to fields (fields left out keep their defaults) and returns the entity. It also
builds the engine's components, such as `aylo.Collider` or `aylo.TextRenderer`. A command that names an unknown
component, or that a client system may not write, is an error and records nothing.

`add` and `patch` differ in what they keep:

| Command | The component afterwards |
|---|---|
| `ctx.commands:add(entity, name, fields)` | Its defaults with `fields` written over them: the value the entity had is replaced, and the entity gets the component if it did not have it. |
| `ctx.commands:patch(entity, name, fields)` | The value the entity has when the commands are applied, with only `fields` written over it. Nothing happens if the entity does not have the component then. |

Use `patch` to change one field of something you did not build yourself, such as the position of a
[capsule](capsules.md) you just spawned:

```luau
local bydo = ctx.call("aylo.capsule.spawn", { capsule = "actors/bydo.actor" })
ctx.commands:patch(bydo, "aylo.Transform", { position = { x = 820, y = 140 } })   -- its rotation and scale stay
```

A patch sees the commands recorded before it in the same system, like that spawn. A field is written whole: a
`position` takes both `x` and `y`, and a list is replaced by the one you give. `aylo.Parent` and `aylo.Children`
cannot be patched; use `setParent` and `removeParent`.

### Events

An event is a message between systems. Declare the ones you use under `events`, then:

```luau
events = { read = { "aylo.CollisionStarted" }, send = { "aylo.PlayMusic" } },
run = function(ctx)
	for collision in ctx.events.read("aylo.CollisionStarted") do
		print(collision.first, collision.second)
	end
	ctx.events.send("aylo.PlayMusic", { sound = music, crossfade = 1 })
end
```

Events are local to one machine and are not replicated: to tell the other side something, put it in a replicated
component. Declaring your own event types from Luau is not available yet (*planned*); you use the engine's, such as
`aylo.CollisionStarted`, `aylo.PlayMusic`, and the network ones in [Networking](networking.md).

## Where a system runs

A system lives in the file that declares it, and a file runs only where an entry requires it. Put what only the server
needs under the `server` entry and what only a client needs under the `client` entry, so a file of the client never
runs on the server and needs no test of `engine.isClient`. The components and input actions both sides must declare
alike go in a file that the shared `main` entry requires, see [Getting started](getting-started.md#your-scripts).

A system's `side` still decides where it runs. A client system may not write replicated or server-only components.

A file that both sides load (one the `main` entry requires) may declare systems of either side. A system is only
checked on the side it runs on: a `side = "client"` system can name the sprites, the sounds, a client-only component
or event and the systems it follows (`after`), and still load on a server, which has none of them. On the side where
it does not run, it is only its name, so the systems that run there can still be ordered after or before it. On its
own side, a name that does not exist is an error, as always (`System 'game.Bad': unknown component 'game.Nope'`):

```luau
-- A shared file: loads on the server and on every client, with no test of engine.isClient.
engine.ecs.component("game.Explosion", { fields = {}, presence = "clientOnly" })
engine.ecs.system("game.RemoveExplosions", {
	side = "client",
	phase = "Update",
	query = { read = { "game.Explosion" } },
	run = function(ctx)
		for entity in ctx.query do
			ctx.commands:despawn(entity)
		end
	end,
})
```

See [Networking](networking.md).

## Errors

A bad declaration (an unknown option, a missing `phase`, an undeclared name) is an error naming the system, raised when
the script loads, and the script fails to load. A mistake inside `run` (an undeclared resource, a call that was not
listed, a stale view) is a Luau error naming the cause. Read the output of the program.

A `run` that never ends or allocates without limit is stopped too, after `scripts.timeout` seconds (5 by default) or
past `scripts.memoryLimit` megabytes (1024), with an error naming the system; the other systems and the next frames
go on. See [Script limits](project-settings.md#script-limits).
