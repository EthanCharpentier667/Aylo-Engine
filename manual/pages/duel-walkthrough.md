# Walkthrough: AYLO Duel

AYLO Duel is a complete two-player game written only in Luau, in `examples/games/aylo_duel/game/`. Two players sit
on either side of an arena. Each opens the game on a main menu, picks **Quick join** to take a free seat, moves, and
clicks to fire lasers; the first to hit the other wins the round, and the next round starts a few seconds later. This
page reads it file by file.

```
game/
  project.luau            settings
  scripts/
    shared/               required by both sides
      main.luau           the shared entry: requires rules.luau
      arena.luau          the constants (a frozen table)
      rules.luau          the replicated components, the input actions, the round, the join packets, the
                          components the capsules name
    behaviours/           Behaviours, run on every side without a require
      Lifetime.luau       removes a misfire after a moment (server)
      Explosion.luau      removes an explosion once it played (client)
      BackgroundPlanet.luau  places a passing planet and removes it (client)
    server/               gameplay (runs on the server)
      main.luau           the server entry
      weapons.luau        firing a laser or misfiring
      players.luau        gives a seat and a ship to each client that asks
      round.luau          walks the round: waiting, live, over, the next one
      ships.luau          moving and firing
      lasers.luau         flying, hitting, winning
    client/               presentation (runs on the clients)
      main.luau           the client entry
      assets.luau         the ids of the atlases, font, sounds, map and menu images the scripts use
      scene.luau          the background and the planets that pass
      sprites.luau        looks of ships and lasers, the loser's explosion
      sounds.luau         music and shots
      menu.luau           the main menu and the quick join, as the client's own screens
      hud.luau            writes the names and the round's line, shows the winner
      controls.luau       a local back key
  assets/                 images, sounds, font, Tiled map, the menu's pixel art (ui/)
    scenes/arena.scene    the arena: the walls and the rock, the camera, the names and the round's line
    actors/               the walls and the rock; the ships, the laser and the misfire the server spawns;
                          the explosion and the planets a client spawns
    atlases/              the frames and animations of the ships, the laser and the explosion (.atlas files)
```

The arena and everything spawned in it are [capsules](capsules.md): files you can open and change in the
[editor](editor.md) (`./aylo-editor examples/games/aylo_duel/game` opens the arena). What a thing does over time is a
[Behaviour](behaviours.md) on its actor when it concerns that thing alone (a misfire that goes after a moment), and a
system when it orders against the engine or reads the whole match (moving the ships between the inputs and the
physics).

Nothing runs by itself: `project.luau` names three entries, and every other file runs because one requires it.

```luau
engine.settings.scripts.main = "shared/main"
engine.settings.scripts.server = "server/main"
engine.settings.scripts.client = "client/main"
```

The server runs `shared/main`, then `server/main`; a client runs `shared/main`, then `client/main`. Files share data
only through `require`: each has its own globals.

## Run it

```sh
./aylo-runtime --server examples/games/aylo_duel/game
./aylo-runtime examples/games/aylo_duel/game      # twice
```

Or `xmake f -y --aylo_duel=y` then `xmake run aylo_duel server` and `xmake run aylo_duel client`. Each client opens on
the main menu; click **Quick join** in both. Escape goes back one screen: from the match to the menu (giving the seat
up), from the menu out of the game.

## `project.luau`: the data that picks the engine

```luau
engine.settings.network.port = engine.args.port or 4242
if engine.isClient then
	engine.settings.network.host = engine.args.host or "127.0.0.1"
end

engine.settings.plugins = {
	"aylo.Physics", "aylo.Capsule", "aylo.Input", "aylo.graphics", "aylo.Shader", "aylo.Sprite",
	"aylo.SpriteDraw", "aylo.Shape", "aylo.Tilemap", "aylo.Text", "aylo.Particle", "aylo.audio",
	"aylo.PhysicsDebug", "aylo.Ui",
}
engine.settings.physics.gravity = { 0, 0 }
```

The port and host come from the command line. A top-down arena needs the physics with no gravity, and the debug view to
see the colliders (F3). Both sides list the same plugins, including what a client draws, plays and reads and the
[user interface](ui.md) of its menu: the capsules name a sprite, a shape or a text, and a server reads every capsule
file too (see [Sides](capsules.md#sides)). A server adds those plugins without their drawing. Clients
also choose `platform = "glfw"`, `renderer = "vulkan"`, a window and an audio backend.
See [Project settings](project-settings.md).

## `shared/`: the rules both sides share

`arena.luau` returns the constants (`shipSpeed`, the collision layers) in a table, which `require` freezes, so no file
can change the rules for the others. A file that needs them starts with
`local Duel = require("@scripts/shared/arena")`.

`rules.luau`, which `shared/main.luau` requires, declares the replicated components:

```luau
engine.ecs.component("game.Ship", {
	fields = { position = "vec2", player = "u32" },
	replication = "everyChange",
})
engine.ecs.component("game.PlayerName", { fields = { player = "u32" }, presence = "clientOnly" })
```

`game.Ship`, `game.Laser`, `game.Winner` and `game.Misfire` are replicated: the clients see them appear and move.
`game.PlayerName` and `game.Banner` mark the texts of the arena's scene: they live on the clients only, but the server
reads the scene too, so it must know them. Then it declares the inputs, once, for both sides:

```luau
engine.input.action("move", "vector2", { bind = { { up = "W", down = "S", left = "A", right = "D" }, ... } })
engine.input.action("shoot", "button", { bind = { "MouseLeft" } })
```

The round is a [state machine](states.md) the server owns: it changes it, and every client follows. The quick join is
three [packets](networking.md#your-own-messages): a client asks for a seat, the server answers, and a client that goes
back to the menu gives its seat up.

```luau
engine.ecs.state("game.Round", { "Waiting", "Live", "Over" }, { replicate = true })

engine.net.packet("game.Join", { direction = "toServer", fields = {} })
engine.net.packet("game.Leave", { direction = "toServer", fields = {} })
engine.net.packet("game.Seat", {
	direction = "toClients",
	fields = { { "granted", "bool" }, { "player", "u8" } },
})
```

See [Components](components.md) and [Setup forms](setup-forms.md).

## `behaviours/`: what a thing does by itself

The files of `scripts/behaviours/` run on every side before the entries, with no `require`. Each declares a
[Behaviour](behaviours.md), and an actor gives it to its entity:

```luau
local Lifetime = engine.behaviour("game.Lifetime", {
	fields = { seconds = "f32" }, -- left to live
	defaults = { seconds = 1 },
	presence = "serverOnly",
	side = "server",
})

function Lifetime:onFixedUpdate(dt)
	self.seconds -= dt
	if self.seconds <= 0 then
		self:destroy()
	end
end
```

```
# assets/actors/misfire.actor
[node path="."]
game.Misfire = {}
game.Lifetime = { seconds = 0.25 }
```

- **`Lifetime.luau`** (server) removes a misfire after a quarter of a second, long enough for a snapshot to carry it.
- **`Explosion.luau`** (client) removes the explosion once its `aylo.SpriteAnimator`, played once, stops `playing`.
- **`BackgroundPlanet.luau`** (client) gives a passing planet, in `onStart`, a random height and distance (the closer,
  the bigger and the faster, through its `aylo.Parallax` scroll), and removes it in `onUpdate` once it left the view.

A Behaviour declares no `before` or `after`. When a system writes a component a Behaviour's hook uses too, the system
orders itself on the hook's name: `sprites.luau` runs `game.FollowLasers` with
`before = { "game.BackgroundPlanet.onUpdate" }`, as both touch `aylo.Transform2D`.

## The server

`server/main.luau` requires the server files below. A client never loads them, so they need no test of
`engine.isServer`. `players.luau`, `round.luau` and `ships.luau` still return when `Duel.networked`
is false: in the editor, the server and the client are one process with no network, so no player joins.

- **`weapons.luau`** has the `fire` function the ship system calls.

`shared/rules.luau` loads the arena on every side in a `Startup` system, with `engine.capsule.load(ctx, Duel.arena)`.
Its walls and its rock are `side=both` nodes: each side spawns its own copy, static, with its collider and its
`aylo.ShapeRenderer`, so nothing about them goes over the network. Its camera and its texts are `side=client` nodes.

- **`players.luau`** reads the `game.Join` packets with `engine.net.received` and gives each asking client a free seat:
  it spawns a ship from its actor (`actors/ship_left.actor` or `ship_right.actor`: `game.Ship`, a `aylo.Transform2D`, a
  box `aylo.Collider` and a kinematic `aylo.RigidBody`) with `engine.capsule.spawn`, then adds `net.Owner` and
  `net.PlayerInput` with `ctx.commands:add`, as an override only changes what the actor has. It answers every request
  with `game.Seat`, sent to that client only: the seat, or `granted = false` when both are taken. It removes the ship
  of a client that sends `game.Leave` or that the engine reports in `net.ClientDisconnected`. A client that is
  connected but did not ask has no ship: it watches the match from its menu. See [Networking](networking.md).
- **`round.luau`** walks `game.Round` with `engine.state.get` and `engine.state.set`. An `Update` system starts the round
  (`Live`) once both seats are taken, and goes back to `Waiting` when a player leaves. On entering `Live` (`onEnter`),
  each ship is spawned again from its actor and the lasers left in flight vanish, so every round starts clean. On
  entering `Over`, a clock starts; a `FixedUpdate` system that runs only `inState` `Over` counts it down and sets
  `Waiting`, which starts the next round at once if both players are still there. On leaving `Over` (`onExit`), the
  `game.Winner` goes. See [States](states.md).
- **`ships.luau`** moves the ships in `FixedUpdate`, only while the round is `Live` (`inState = { "game.Round",
  "Live" }`), after `net.applyInputs` and before the physics syncs, reading the
  player's `move` action with `engine.input.read(ctx, entity, "move")` and sliding along the obstacles with
  `engine.physics.moveAndSlide`. On `shoot.pressed` it spawns `actors/laser.actor` (a dynamic body with a
  `aylo.Velocity`, no gravity and continuous collision), with overrides for its place, its shooter, its direction and
  the ship it can hit. One shot in ten misfires: `actors/misfire.actor` replaces the laser so the clients play
  another sound, and its `game.Lifetime` removes it.
- **`lasers.luau`** copies the laser bodies' positions into the replicated `game.Laser` after the physics, removes
  those that left the view, and reads `aylo.CollisionStarted` events: a laser that touches something is spent, and
  while the round is `Live`, one that touches a ship spawns a `game.Winner` and sets the round to `Over`. See
  [Physics](physics.md).

## The client

`client/main.luau` requires the client files below, so the server never loads them.

- **`assets.luau`** returns the ids of what the client's scripts use, each loaded by its `engine.assets` form: the
  ships' and the laser's atlases (`engine.assets.atlas({ path = "atlases/ship_left.atlas" })`: their frames and
  animations are in [`.atlas` files](setup-forms.md#atlas-files)), the font, the sounds (the music with
  `stream = true`), the map and the menu images; the files that draw require it. What only a capsule names needs no
  line: the explosion's atlas file, the planets' images and the arena's font are loaded by the capsules that name
  them. See [Setup forms](setup-forms.md) and [Capsule files](capsules.md#capsule-files).
- **`scene.luau`** spawns the space background map, and now and then a planet's actor with `engine.capsule.spawn`.
- **`sprites.luau`** gives the ships and the lasers, which the server spawned on its side only, a
  `aylo.SpriteRenderer` and `aylo.SpriteAnimator` from the atlases, in `PostUpdate` (which also runs while the editor
  edits, so an actor opened alone shows its look), and spawns `actors/explosion.actor` where the loser was.
- **`sounds.luau`** sends `aylo.PlayMusic` at start and calls `engine.audio.play` for each new laser or misfire.
- **`menu.luau`** declares the client's own state machine, `game.Screen` (`Menu`, `Joining`, `Playing`), with no
  `replicate`: each client has its own. Entering `Menu` or `Joining` spawns that screen with `engine.ui.spawn` (the
  arena dimmed, a panel, the buttons), and leaving it despawns every node marked with a `game.ScreenNode`. **Quick
  join** enters `Joining`, where a system sends `game.Join` as soon as `engine.net.localClient` says the server
  answered, and reads the `game.Seat` reply: a seat enters `Playing`, a refusal (or no server after five seconds)
  goes back to `Menu`, which says why. The UI pack's buttons are images: a system switches each button's
  `aylo.UiImage` texture by its `aylo.UiInteraction` (`hovered`, `pressed`), and another steps the loading wheel's
  `source` through its frames. See [User interface](ui.md).
- **`hud.luau`** writes the texts of the arena's scene: the names (marking this player's) and the line at the bottom that
  says where the round is (`Waiting for an opponent...`, `Next round in 3`), and when `game.Winner` appears says who
  won, using `engine.net.localClient` and `net.Owner` to know whether this client is the winner (with the network
  only). Those texts go when the client follows the round out of `Over` (`onExit`).
- **`controls.luau`** adds a local Escape action, `back`, with engine calls instead of `engine.input.action`, so it is
  not sent to the server; `menu.luau` reads it.

## What to take from it

1. The server decides everything that matters and writes replicated components; the clients only present them. The
   phase of the match is a state the server owns (`replicate = true`); a client's screens are a state of its own.
2. One folder per side with an entry each, shared declarations in `shared/`, and data shared through `require`.
3. Everything the engine does for the game is chosen in data: plugins and settings in `project.luau`, assets and
   inputs by forms, what is in the world by capsules, behavior by Behaviours and systems.
