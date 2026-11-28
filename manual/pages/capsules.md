# Capsules

A capsule is an entity your game describes once and spawns as often as it wants: its components, and its child
entities, any of which can be another capsule. An enemy, a bullet, a boss with its turrets and a whole level are all
capsules. Name the file after what it is for: `.actor` for something that moves, `.scene` for a level, `.capsule` for
anything else. The engine treats them the same way.

Add the plugin in `project.luau`:

```luau
engine.settings.plugins = { "aylo.Capsule" }
```

## What a capsule holds

A capsule is a tree of nodes. Each node becomes one entity when the capsule is spawned:

- its **components**, with their values;
- its **children**, parented to it, so a turret follows its boss;
- its **side**: the server by default, `client` for what only players see (a background, a camera), or `both`;
- optionally, **another capsule** it spawns instead of a plain entity: a boss's turret node spawns
  `actors/turret.actor`, and the node's own components replace the turret's (its position on the boss).

A level is a capsule whose root has no `aylo.Transform2D`: its children stay where the level put them. A capsule that
should move as one gives its root a `aylo.Transform2D`.

## Capsule files

Put your capsules anywhere under `assets/`. The engine reads every `.capsule`, `.scene` and `.actor` file when the game
starts, after your scripts, so a file can use the components they declare. A capsule's name is its path from
`assets/`, for example `actors/boss.actor`.

The models (`.gltf`, `.glb`, `.fbx`, `.obj`), materials (`.material`), images (`.png`, `.jpg`, `.bmp`, `.tga`,
`.hdr`), [atlas files](setup-forms.md#atlas-files) (`.atlas`), fonts (`.json`) and sounds (`.wav`, `.mp3`, `.ogg`,
`.flac`) a capsule names are loaded when its file is read, and again when
you save it: `aylo.MeshRenderer = { mesh = "models/statue.obj" }` draws without an `engine.assets.mesh` in a script,
and `aylo.AudioSource = { sound = "sfx/engine.ogg", loop = true }` plays without an `engine.assets.sound`. An asset
loads with the defaults of its setup form: an image pixelated and not repeated, a sound decoded once. A music that
streams says so in its [import file](setup-forms.md#import-files), `maint.ogg.import`, and then streams wherever it is
named. For other options (a smooth image), load it in a script with them first: the first load wins. A file that
cannot be loaded is reported on the output, and the game goes on.

!!! note "A `.json` in a capsule is a font"
    A `.json` file a capsule names is loaded as a font (`engine.assets.font`). To name an Aseprite or TexturePacker
    atlas in a capsule, give it a [`.atlas` file](setup-forms.md#atlas-files) that points to the export:
    `{ "path": "ship.json" }`.

```
[capsule format=1]

# The boss, with a turret on each side.
[node path="."]
game.Health = { value = 50 }
aylo.Transform2D = {}

[node path="LeftTurret" instance="actors/turret.actor"]
aylo.Transform2D = { position = (-40, 0) }

[node path="RightTurret" instance="actors/turret.actor"]
aylo.Transform2D = { position = (40, 0) }

[node path="Shadow" side=client]
aylo.SpriteRenderer = { texture = "sprites/boss_shadow.png", layer = -1 }
```

Each `[node]` is one entity: `path="."` is the root, `instance` places another capsule there (write only the fields
you change), and `side=client` keeps a node on the players' machines. The
[Capsule file format](capsule-format.md) page has the whole format: every attribute, how to write each field type,
instances and the error messages.

With [hot reload](hot-reload.md) on, saving a file respawns what came from it in place, and a file saved with a
mistake keeps its previous version. [Reading and writing capsule files](capsule-files.md) says when the engine reads
your files, what saving one does to a running game, and what a file looks like once the engine writes it back.

## Names

Each entity a capsule spawns gets a `aylo.Name`: the name of its node (`LeftTurret`), or for the root, the file's name
without its folder and extension (`boss` for `actors/boss.actor`). A node that gives a `aylo.Name` of its own keeps it.
The [editor](editor.md) shows entities by these names; a script can read them like any component.

## Spawning, switching levels, reloading

The `engine.capsule` forms do each of these in one line, from a system's `run`:

```luau
engine.ecs.system("game.Waves", {
	phase = "Update",
	query = {},
	run = function(ctx)
		local bydo = engine.capsule.spawn(ctx, "actors/bydo.actor", {
			["aylo.Transform2D"] = { position = { x = 820, y = 140 } },
		})

		if bossDefeated then
			engine.capsule.load(ctx, "scenes/level2.scene")
		end
	end,
})
```

| Form | The call it makes | Returns | What it does |
|---|---|---|---|
| `engine.capsule.spawn(ctx, path, overrides)` | `aylo.capsule.spawn` | the root entity | Spawns a copy of the capsule |
| `engine.capsule.load(ctx, path, overrides)` | `aylo.capsule.load` | the root entity | Switches levels: the capsule loaded before is despawned, then this one is spawned |
| `engine.capsule.reload(ctx, root)` | `aylo.capsule.reload` | `true` if `root` was a capsule's root, `false` otherwise | Spawns the capsule's current version again into the same root |

`path` is the capsule file's path in `assets/`. `overrides` is optional: a table of component name to fields, written
over the capsule root's values. **Only the fields you name change**: in the example the bydo keeps the rotation, the
scale and every other component its file gives it. A component the root does not have is not added by an override;
add it with `ctx.commands:add`. To change a child, find it with a query once it is spawned.

- The root is ready to use at once, to keep in a table or pass to other commands.
- The entities appear when the system's commands are applied, at the end of its phase, like `ctx.commands:spawn`.
- Despawning the root (`ctx.commands:despawn(root)`) removes the whole capsule.
- `load` only removes the capsule loaded before: your player and anything else spawned outside it stay.
- A reload despawns the root's children, including ones you attached to it, and keeps the root's other components.

### The calls underneath

The forms are thin: `engine.capsule.spawn(ctx, path, overrides)` is the call followed by one
[`ctx.commands:patch`](systems.md#commands) per override. You can make the calls yourself:

| Call | Arguments | Result |
|---|---|---|
| `aylo.capsule.spawn` | `capsule` (a path) | the root entity |
| `aylo.capsule.load` | `capsule` (a path) | the root entity |
| `aylo.capsule.reload` | `root` (an entity) | `true` if it was a capsule's root |

```luau
local bydo = ctx.call("aylo.capsule.spawn", { capsule = "actors/bydo.actor" })
ctx.commands:patch(bydo, "aylo.Transform2D", { position = { x = 820, y = 140 } })
```

## Sides

Each node is spawned on its side only, so one capsule loads on the server and on every client without duplicates: the
server's entities reach the players by replication, and the client nodes stay local. Components marked server-only
are left out on clients, and client-only ones on a dedicated server.

Every side reads every capsule file, its client nodes too, so every component a file names must be declared on the
server as well: a component of yours in a shared script, and a plugin's by listing the plugin on both sides. A server
adds the drawing plugins (`aylo.Sprite`, `aylo.Shape`, `aylo.Text`...) without their drawing, so a sprite in a capsule
costs it nothing. A file that names a component the side does not know stops the game with `unknown component`.

A scene both sides load (`engine.capsule.load` in a `side = "both"` system) spawns its client nodes on the clients,
and its `both` nodes on every side, each its own copy (a static wall needs no network).

### Actors the server spawns

An actor the server spawns while the game runs (`engine.capsule.spawn` in a server system) reaches every client with
its look, as a prefab does in other engines: write the sprite in the actor, not in a client script.

```
[capsule format=1]

# actors/ship_left.actor: the server moves it; every client draws it.
[node path="."]
game.Ship = { player = 0 }
aylo.Transform2D = {}
aylo.Collider = { size = (48, 48) }
aylo.SpriteRenderer = { layer = 1 }
aylo.SpriteAnimator = { atlas = "atlases/ship_left.atlas", animation = "fly" }

[node path="Flame" side=client]
aylo.Transform2D = { position = (-30, 0) }
aylo.SpriteRenderer = { layer = 0, texture = "fx/flame.png" }
```

When the actor arrives on a client, the client spawns the part the server does not send:

| Part of the actor | On a client |
|---|---|
| The root's replicated components (`game.Ship`) | from the server, as it changes them |
| The root's other components (`aylo.SpriteRenderer`, `aylo.Collider`, `aylo.Transform2D`...) | from the file, once |
| `side=client` nodes, and their children | spawned by the client, under the root |
| `server` and `both` nodes | the server's: they arrive with their replicated components only |
| Overrides given to `engine.capsule.spawn` | only those of replicated components |

This needs a root with a replicated component of yours (`game.Ship` above): an actor whose root has none stays on the
server, and nothing of it reaches the clients. A client never steps the physics, so the colliders it gets only follow
their entity (the debug view shows them).

!!! warning "What differs per actor must be replicated"
    The client reads the root's other components from the file, not from the server. A position the server gives at
    spawn (`{ ["aylo.Transform2D"] = { position = ... } }`) does not reach the client: replicate it in a component of
    yours (`game.Ship.position`) and copy it into `aylo.Transform2D` in a client system, as AYLO Duel does.

## Errors

A call on a path no capsule has, or a capsule that contains itself through its instances, is a Luau error naming the
call. Nothing is spawned then. The forms start their errors with their name:

| Message | Cause |
|---|---|
| `engine.capsule.spawn: expected the system's ctx (or a Behaviour's self) first` | The first argument is not the `ctx` of `run` |
| `engine.capsule.spawn: expected the capsule's path second ("actors/bydo.actor")` | The path is missing, empty or not a string |
| `engine.capsule.spawn: overrides: 'aylo.Transform2D' must be a table of fields` | An override is not a table |
| `engine.capsule.spawn: overrides: ...` followed by the field's error | An override names a field the component does not have, or a value that does not fit; the capsule is spawned without it |
| `engine.capsule.spawn: ...` followed by the call's error | An unknown capsule, or the call missing from the system's `calls` |

`engine.capsule.load` and `engine.capsule.reload` give the same messages with their own name.
