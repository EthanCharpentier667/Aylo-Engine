# Systems

A system is a function that runs on a schedule, on the entities that match a query. You declare one with
`engine.ecs.system`. To attach a script to entities without writing a query, see [Behaviours](behaviours.md), which
are made of systems.

```luau
engine.ecs.system("game.MoveShips", {
	phase = "FixedUpdate",
	side = "server",
	query = { write = { "game.Ship", "aylo.Transform2D" }, with = { "net.PlayerInput" } },
	after = { "net.applyInputs" },
	run = function(ctx)
		for entity, ship, transform in ctx.query do
			local move = engine.input.read(ctx, entity, "move").vector
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
| `resources` | Optional. `{ read = {...}, write = {...} }`: resources it uses, checked when the script loads. See [What a system uses](#what-a-system-uses). |
| `components` | Optional. `{ read = {...}, write = {...} }`: components it reads or writes on [other entities](#other-entities) with `ctx:get`, checked when the script loads. |
| `events` | Optional. `{ read = {...}, send = {...} }`: events it reads or sends, checked when the script loads. |
| `calls` | Optional. [Engine calls](engine-calls.md) it makes, checked when the script loads. |
| `after`, `before` | Names of systems it must run after or before. |
| `inState`, `onEnter`, `onExit` | `{ machine, value }`: runs only in a game state, or once when the game enters or leaves it. See [States](states.md). |

Any other option is an error naming the system.

## What a system uses

A system can use any resource, event, [engine call](engine-calls.md) or component of [another entity](#other-entities)
without listing it: the engine finds what it
uses the first time it uses it. This is what lets a helper function make calls of its own, without the systems that
call it knowing about them:

```luau
-- scripts/server/weapons.luau
local Weapons = {}

function Weapons.fire(ctx, position)
	engine.capsule.spawn(ctx, "actors/laser.actor", { ["aylo.Transform2D"] = { position = position } })
end

return Weapons
```

```luau
-- scripts/server/ships.luau: no `calls` needed for the spawn inside Weapons.fire
engine.ecs.system("game.Fire", {
	phase = "FixedUpdate",
	query = { read = { "game.Ship" } },
	run = function(ctx)
		for entity, ship in ctx.query do
			Weapons.fire(ctx, ship.position)
		end
	end,
})
```

What the engine finds:

| You use | The system then |
|---|---|
| `ctx.call(name, ...)`, or a form that makes the call | makes the call, from now on as if it had listed it |
| `ctx.resources[name]` | reads the resource; the first time you write a field of it, it writes it |
| `ctx:get(entity, name)`, `ctx:has(entity, name)` | reads the component; the first time you write a field of it, it writes it |
| `ctx:getParent(entity)`, `ctx:getChildren(entity)` | reads `aylo.Parent` or `aylo.Children` |
| `ctx:isEnabled(entity)` | reads `aylo.Disabled` and `aylo.Parent` |
| `ctx.events.read(name)` | reads the event, from the oldest one still queued |
| `ctx.events.send(name, ...)` | sends the event |

Listing something is still allowed, and it does two things:

- **A name you list is checked when the script loads.** A misspelled resource in `resources` stops the script at once,
  instead of failing the first time `run` reaches that line.
- **A name you list keeps the limits you gave it.** A resource under `resources.read` or a component under
  `components.read` stays read-only, and an event under `events.read` cannot be sent: writing or sending it is an
  error.

The [query](#queries) is different: you always declare it, since it says which entities the system visits.

### Two systems on the same data

Two systems of the same phase that use the same data, one of them writing it, must say which runs first, with `after`
or `before`. For what a system lists, a missing order is an error when the game starts. For what the engine finds while
the game runs, it is a warning, written once:

```
[Schedule] Systems of phase Update touch the same data with no order between them, found while they ran (not declared): 'game.Fire' and 'game.Score' both use resource 'game.Points', one of them writing it. They run in the order they were added; order them with before / after
```

The game goes on: the systems keep running in the order they were added. Add `after` or `before` to one of them to
make the order yours. In single player and in the editor, a server system that uses the data of a client system runs
before it, as over the network, so that pair needs no order.

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
its `aylo.Transform2D`, and the engine draws it between its last two ticks instead:

```luau
engine.ecs.system("game.SpawnBall", {
	phase = "Startup",
	side = "server",
	run = function(ctx)
		ctx.commands:spawn({
			["aylo.Transform2D"] = { position = { x = 400, y = 300 } },
			["aylo.TransformInterpolation"] = {},
		})
	end,
})
```

The component has no fields. On the client (and in single player and the editor), after the ticks of each frame, the
engine records the entity's `aylo.Transform2D` and computes the position you see, `aylo.GlobalTransform2D`, from the last
two records, blended by how far the frame is into the next tick. Its children follow the blended position.

- **What is drawn is up to one tick behind** the simulation (about 17 ms at 60 ticks a second). `aylo.Transform2D`, the
  physics and the server keep the simulated position: read `aylo.Transform2D` for gameplay.
- **A move made out of the ticks is drawn at once**: a teleport written in `Update`, or an entity moved in the editor,
  does not slide there.
- **Add it only to what moves in `FixedUpdate`** (your gameplay, or a body of the [physics](physics.md)). An entity a
  client moves in `Update`, such as one following replicated data, gains nothing from it.
- The server never blends: a dedicated server draws nothing, so its `aylo.GlobalTransform2D` is the `aylo.Transform2D`.

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
	read = { "aylo.Transform2D" },
	write = { "game.Ship" },
	optionalRead = {}, optionalWrite = {},
	with = { "net.Owner" }, without = { "game.Dead" },
	added = { "game.Laser" }, changed = { "game.Ship" },
	removed = { "game.Enemy" },
}
```

| Term | Meaning |
|---|---|
| `read`, `write` | The entity must have the component, and the loop gets a view of it (read-only, or writable). |
| `optionalRead`, `optionalWrite` | A view, or `nil` when the entity does not have it. A tag it has is a view with no fields, so `if dead then` tests it. |
| `with`, `without` | The entity must have, or must not have, the component. No view. |
| `added` | The entity must have it, and it was added since this system last ran. |
| `changed` | The entity must have it, and it was added or written since this system last ran. |
| `removed` | The entity lost it since this system last ran: it was removed, or the entity was despawned. No view. |

`for entity, a, b in ctx.query do` gives the entity, then one view per `read`/`write` term in the order read, write,
optionalRead, optionalWrite (inside each, in the order you listed them). The views are valid only during the call to
`run`. An unknown term name is an error.

### Views by name

Adding a `read` to a query shifts every variable of a `ctx.query` loop after it. `ctx.views` gives the same entities
with their views by name instead, in one table, so the loop does not change when the query does:

```luau
engine.ecs.system("game.Regen", {
	phase = "Update",
	query = { read = { "game.Position" }, write = { "game.Shield" }, optionalRead = { "game.Dead" } },
	run = function(ctx)
		for entity, it in ctx.views do
			if it.dead == nil then
				it.shield.strength += it.position.x
			end
		end
	end,
})
```

Each view is there under the component's name, `it["game.Shield"]`, and under its short name: what follows the last
`.`, its first letter in lower case (`game.Shield` is `it.shield`, `aylo.Transform2D` is `it.transform2D`). When two
terms have the same short name (`game.Position` and `net.Position`), use the full names: `it.position` is then an
error that says so. An optional view the entity lacks is `nil`; a name the query does not have is an error listing
the ones it has.

`it` is one table for the whole loop, filled again for each entity: it costs nothing per entity, but do not keep it,
nor its views, past the step. Use `ctx.query` or `ctx.views`, whichever reads better; they visit the same entities.

### Reacting to a removal

`removed` visits each entity that lost a component, once, the next time the system runs: to drop loot when an enemy
dies, or end an effect when a buff is gone.

```luau
engine.ecs.system("game.DropLoot", {
	phase = "Update",
	query = { removed = { "game.Enemy" } },
	run = function(ctx)
		for enemy in ctx.query do
			-- enemy lost game.Enemy, or was despawned: its components may be gone
			ctx.commands:spawn({ ["game.Loot"] = {} })
		end
	end,
})
```

There is no view of the removed component: its value is gone. A despawned entity is dead in the loop, so it has no
component at all, and the other terms are checked on the entity as it is now: `removed = { "game.Enemy" }` with
`read = { "aylo.Transform2D" }` only visits the entities that lost `game.Enemy` and are still alive with a transform.
To place the loot where a despawned enemy was, record what you need (its position) before you despawn it. An entity
that lost the component and got it back since is still visited.

### Switching an entity off

`aylo.Disabled` switches an entity off without despawning it, its children with it: your systems skip it, it is not
drawn, its body leaves the physics and its sound stops (it starts over when the entity is back). It keeps its components, and comes back as it was when you
remove the tag:

```luau
ctx.commands:add(enemy, "aylo.Disabled", {})   -- off, with its children
ctx.commands:remove(enemy, "aylo.Disabled")    -- on again
```

Every query skips the entities that have `aylo.Disabled`, or whose parent, or a parent of it, has it. A query that names
`aylo.Disabled` in any term sees them all, which is how you find the ones to turn back on:

```luau
engine.ecs.system("game.WakeUp", {
	phase = "Update",
	query = { with = { "game.Enemy", "aylo.Disabled" } }, -- names aylo.Disabled: sees the ones switched off
	run = function(ctx)
		if ctx.time.elapsed < 3 then
			return
		end
		for enemy in ctx.query do
			ctx.commands:remove(enemy, "aylo.Disabled")
		end
	end,
})
```

`aylo.Disabled` is replicated: only the server switches an entity off, and its clients hide it too. `ctx:get` still
reads a switched-off entity: only queries skip it. `ctx:isEnabled(entity)` makes the test a query makes: it is `false`
when the entity or one of its parents has `aylo.Disabled` (and `true` for an entity that is gone). In the editor, the Inspector's **Active** box adds or removes it, and
a capsule saves it like any component.

## Other entities

The query gives you the components of the entities it visits. To reach any other entity, the one a bullet hit, the
target a ship follows or the parent of a turret, ask `ctx` for one of its components by name:

```luau
engine.ecs.system("game.Damage", {
	phase = "FixedUpdate",
	after = { "aylo.Physics.Events" }, -- once this frame's collisions are sent
	run = function(ctx)
		for collision in ctx.events.read("aylo.CollisionStarted") do
			local health = ctx:get(collision.second, "game.Health") -- a view, or nil
			if health then
				health.value -= 1
			end
		end
	end,
})
```

| Call | Gives |
|---|---|
| `ctx:get(entity, name)` | A view of the entity's component, read and written like those of the query, or `nil` when the entity is gone or does not have it. |
| `ctx:has(entity, name)` | `true` when the entity is alive and has the component, `false` otherwise. |

Call them with `:` (`ctx:get`, not `ctx.get`). The system reads the component from its first `ctx:get`, and writes it
from the first time you write a field of a view; the component is marked changed then, not when you only read it.
Listing it under `components = { read = {...}, write = {...} }` checks the name when the script loads, and a component
under `components.read` gives read-only views. See [What a system uses](#what-a-system-uses).

Some writes are refused, with a Luau error that names the component:

- `aylo.Parent` and `aylo.Children` are read only: change the hierarchy with `ctx.commands:setParent()`.
- A system running on clients reads every component, but cannot write a replicated or a server-only one, as for its
  query (see [Where a system runs](#where-a-system-runs)).

!!! warning "Keep the entity, not the view"
    A view lives only during the `run` that gave it. Store the entity, in a component field of type `entity`
    (`target.entity = collision.second`), and call `ctx:get` again in the next run: a view kept in a variable is an
    error the next time you use it, `the view of 'game.Health' is no longer valid`.

### The hierarchy

An entity attached with `ctx.commands:setParent(child, parent)` has a parent, and the parent lists its children. Walk
them with `ctx:getParent` and `ctx:getChildren`, then read their components with `ctx:get`:

```luau
engine.ecs.system("game.CoolBarrels", {
	phase = "Update",
	query = { with = { "game.Turret" } },
	run = function(ctx)
		for turret in ctx.query do
			for _, child in ctx:getChildren(turret) do
				local barrel = ctx:get(child, "game.Barrel")
				if barrel then
					barrel.heat = math.max(barrel.heat - ctx.delta, 0)
				end
			end
		end
	end,
})
```

| Call | Gives |
|---|---|
| `ctx:getParent(entity)` | The entity's parent, or `nil` when it has none or is gone. |
| `ctx:getChildren(entity)` | A list of the entity's children, in the order they were attached; empty when it has none or is gone. |

They read `aylo.Parent` and `aylo.Children`, like `ctx:get` would, and need nothing declared. The list is a copy: you
may keep it after `run`, but it does not follow later changes. A `setParent` you record shows the next time the system
runs, once the commands are applied.

## `ctx`

| Field | What it gives |
|---|---|
| `ctx.query` | The loop over matching entities, with one view per term. |
| `ctx.views` | The same loop, with the views by name in one table: see [Views by name](#views-by-name). |
| `ctx.side` | The side the system runs on: `"server"` (the default), `"client"` or `"both"`. |
| `ctx.time` | `delta` (seconds of the frame), `fixedDelta` (the fixed step), `elapsed`, `tick` and `alpha` (progress between two fixed steps). `ctx.delta` and `ctx.fixedDelta` also exist. |
| `ctx.commands` | Deferred changes to the world, see below. |
| `ctx.resources` | The resources: `ctx.resources["aylo.Input"]` is a view, `nil` while the resource has no value (read-only when listed under `resources.read`). |
| `ctx.events` | `ctx.events.read(name)` loops over the events not read yet; `ctx.events.send(name, fields)` sends one. |
| `ctx.call(name, args)` | Makes an engine call. Most have a [form](setup-forms.md#forms-you-use-in-a-system) that makes it for you: `engine.input.read(ctx, ...)`, `engine.state.get(ctx, ...)`. |
| `ctx:get(entity, name)`, `ctx:has(entity, name)` | A component of any entity, see [Other entities](#other-entities). |
| `ctx:getParent(entity)`, `ctx:getChildren(entity)` | The parent and the children of an entity, see [The hierarchy](#the-hierarchy). |
| `ctx:isEnabled(entity)` | Whether the entity is switched on, see [Switching an entity off](#switching-an-entity-off). |

### Commands

Changes to the world are recorded and applied after the system, not during the loop (a spawned entity cannot be queried by the loop that spawned it):

```luau
local ship = ctx.commands:spawn({
	["game.Ship"] = { position = position, player = 0 },
	["aylo.Transform2D"] = { position = position },
})
ctx.commands:add(ship, "aylo.RigidBody", { type = "kinematic" })
ctx.commands:patch(ship, "game.Ship", { player = 1 })
ctx.commands:setParent(child, parent) -- read it back with ctx:getParent, see The hierarchy
ctx.commands:removeParent(child)
ctx.commands:remove(ship, "aylo.RigidBody")
ctx.commands:despawn(ship)
```

`spawn` takes a table of component name to fields (fields left out keep their defaults) and returns the entity. It also
builds the engine's components, such as `aylo.Collider` or `aylo.TextRenderer`. A command that names an unknown
component, or that a client system may not write, is an error and records nothing. `remove` takes the component away
when the commands are applied, and does nothing if the entity does not have it then; use `removeParent` for
`aylo.Parent`.

`add` and `patch` differ in what they keep:

| Command | The component afterwards |
|---|---|
| `ctx.commands:add(entity, name, fields)` | Its defaults with `fields` written over them: the value the entity had is replaced, and the entity gets the component if it did not have it. |
| `ctx.commands:patch(entity, name, fields)` | The value the entity has when the commands are applied, with only `fields` written over it. Nothing happens if the entity does not have the component then. |

Use `patch` to change one field of something you did not build yourself, such as the position of a
[capsule](capsules.md) you just spawned:

```luau
local bydo = ctx.call("aylo.capsule.spawn", { capsule = "actors/bydo.actor" })
ctx.commands:patch(bydo, "aylo.Transform2D", { position = { x = 820, y = 140 } })   -- its rotation and scale stay
```

A patch sees the commands recorded before it in the same system, like that spawn. A field is written whole: a
`position` takes both `x` and `y`, and a list is replaced by the one you give. `aylo.Parent` and `aylo.Children`
cannot be patched; use `setParent` and `removeParent`.

### Events

An event is a message between systems. Read and send them with `ctx.events` (listing them under `events` is
optional, see [What a system uses](#what-a-system-uses)):

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
component (or a [packet](networking.md#your-own-messages)). The engine has its own, such as `aylo.CollisionStarted`,
`aylo.PlayMusic`, and the network ones in [Networking](networking.md).

#### Your own events

Declare an event type with `engine.ecs.event`, in a file every side that uses it loads, then send and read it like the
engine's:

```luau
engine.ecs.component("game.Health", { fields = { value = "f32" } })
engine.ecs.event("game.Hit", { fields = { target = "entity", damage = "u32" } })

engine.ecs.system("game.Shoot", {
	phase = "Update",
	query = { with = { "game.Health" } },
	run = function(ctx)
		for enemy in ctx.query do
			ctx.events.send("game.Hit", { target = enemy, damage = 3 })
		end
	end,
})

engine.ecs.system("game.TakeHits", {
	phase = "PostUpdate",
	run = function(ctx)
		for hit in ctx.events.read("game.Hit") do
			local health = ctx:get(hit.target, "game.Health")
			if health then
				health.value -= hit.damage
			end
		end
	end,
})
```

`fields` takes the [field types](components.md#field-types) of components, and nothing else: an event has no
`defaults` (a field you do not send is zero), no `replication` and no `presence`, since it never leaves the machine.
Like a component's, its fields are fixed while the game runs: a [hot reload](hot-reload.md) that changes them, or
stops declaring the event, rebuilds the game. A name cannot be both a component's and an event's.

| Message | Cause |
|---|---|
| `Event 'game.Hit': unknown option 'replication' (an event takes only 'fields')` | Another key than `fields` |
| `Event 'game.Hit': needs a 'fields' table` | No `fields` |
| `Event 'game.Hit': ...` | A field type that does not exist, as for a component |

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

A bad declaration (an unknown option, a missing `phase`, an unknown name in the query or in `calls`, `resources`,
`components` or `events`) is an error naming the system, raised when the script loads, and the script fails to load. A mistake inside
`run` is a Luau error naming the cause. Read the output of the program.

| Message inside `run` | Cause |
|---|---|
| `unknown engine call 'aylo.audio.playSund' (system 'game.Pew')` | No call has this name (a typo, or a plugin the project does not load) |
| `unknown resource 'game.Pints' (system 'game.Score')` | No resource has this name |
| `unknown event 'game.Hitt' (system 'game.Damage')` | No event has this name |
| `unknown component 'game.Helth' (system 'game.Damage')` | No component has this name (`ctx:get`, `ctx:has`) |
| `unknown component 'game.Helth': no script declared it and the world has none` | A command (`add`, `patch`, `remove`, `spawn`) names a component that does not exist |
| `cannot remove 'aylo.Parent': use removeParent` | `ctx.commands:remove` on `aylo.Parent` or `aylo.Children` |
| `a client system cannot remove 'game.Health': it is replicated or server-only` | A client system removes a component the server owns |
| `the query of system 'game.Regen' has no view 'sheild' (it has: position, shield, dead)` | A name `ctx.views` does not have |
| `'position' names several views of system 'game.Check': use it["game.Position"] or it["net.Position"]` | A short name two terms share |
| `the views of system 'game.Regen' are read by name: ...` | Writing a new key in the table of `ctx.views` |
| `ctx:get expects an entity and the name of a component: ...` | `ctx.get` written with a `.`, or an argument that is not an entity or a name |
| `the view of 'game.Health' is read-only: system 'game.Damage' declared it under 'components.read' ...` | The component is listed under `components.read` |
| `a client system cannot write 'game.Health': it is replicated: the server would overwrite it` | A client system writes a field of a replicated (or server-only) component it got with `ctx:get` |
| `ctx:getParent expects an entity: ctx:getParent(entity)` | `ctx.getParent` written with a `.`, or an argument that is not an entity (also `ctx:getChildren`, `ctx:isEnabled`) |
| `'aylo.Parent' cannot be written: use commands:setParent()` | A field of `aylo.Parent` or `aylo.Children` written through `ctx:get` |
| `cannot write 'count': the view of 'game.Points' is read-only` | The resource is listed under `resources.read` |
| `event 'aylo.PlayMusic' is declared to be read by system 'game.Music', not sent: ...` | The event is listed under `events.read` and the system sends it |
| `Call 'x' writes component 'y', which is replicated: ...` | A client system makes a call that writes server data |
| `ctx.resources is no longer valid: ...`, `... is no longer valid` | A view or `ctx.resources` kept after `run` returned |

A `run` that never ends or allocates without limit is stopped too, after `scripts.timeout` seconds (5 by default) or
past `scripts.memoryLimit` megabytes (1024), with an error naming the system; the other systems and the next frames
go on. See [Script limits](project-settings.md#script-limits).
