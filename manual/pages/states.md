# States

Most games move through phases: a menu, a lobby, the match, a game over screen. A **state machine** names those
phases, and your systems say in which one they run, or that they run once when the game enters or leaves one. You
need no plugin for it: every game has states.

```luau
engine.ecs.state("game.Phase", { "Menu", "Playing", "GameOver" })

engine.ecs.system("game.MoveShips", {
	phase = "FixedUpdate",
	inState = { "game.Phase", "Playing" }, -- skipped in the menu and on the game over screen
	query = { write = { "aylo.Transform" }, with = { "game.Ship" } },
	run = function(ctx)
		-- ...
	end,
})
```

## Declaring a machine

`engine.ecs.state(name, values)` declares a machine: its name, like a component's (`game.Phase`), and the list of its
values, as strings. Call it at load time, before the systems that name it, in a file every side that uses it loads.

The machine starts in its **first value**: it enters it at the start of the first frame, so the `onEnter` systems of
that value run then. Declaring it again with the same values does nothing, so a [hot reload](hot-reload.md) keeps the
current value.

## Running in a state

A system takes one of three options, each a pair `{ machine, value }`:

| Option | The system runs |
|---|---|
| `inState` | In its `phase`, on every tick or frame while the machine is in `value`. |
| `onEnter` | Once, when the machine enters `value`. No `phase`. |
| `onExit` | Once, when the machine leaves `value`. No `phase`. |

A system has at most one of them. `onEnter` and `onExit` systems run in a phase of their own, `StateTransition`, which
you never write: give them no `phase`. They take a `query`, `resources`, `events` and `calls` like any system, so they
are where you spawn a level or clear the menu.

```luau
local font = engine.assets.font("fonts/title.json")

engine.ecs.system("game.ShowTitle", {
	onEnter = { "game.Phase", "Menu" },
	side = "client",
	run = function(ctx)
		ctx.commands:spawn({
			["aylo.Transform"] = { position = { x = 400, y = 200 } },
			["aylo.TextRenderer"] = { text = "Press Space", font = font, size = 32 },
			["game.Title"] = {},
		})
	end,
})

engine.ecs.system("game.HideTitle", {
	onExit = { "game.Phase", "Menu" },
	side = "client",
	query = { with = { "game.Title" } },
	run = function(ctx)
		for entity in ctx.query do
			ctx.commands:despawn(entity)
		end
	end,
})
```

## Changing the state

A system changes the state with the engine call `aylo.state.set`, and reads it with `aylo.state.get`. List them in its
`calls`.

| Call | Arguments | Result |
|---|---|---|
| `aylo.state.get` | `state` (the machine) | its current value, `""` before its first frame |
| `aylo.state.set` | `state`, `value` | nothing |

```luau
engine.ecs.system("game.StartOnSpace", {
	phase = "Update",
	side = "client",
	inState = { "game.Phase", "Menu" },
	calls = { "aylo.input.read", "aylo.state.set" },
	run = function(ctx)
		if ctx.call("aylo.input.read", { action = "start" }).pressed then
			ctx.call("aylo.state.set", { state = "game.Phase", value = "Playing" })
		end
	end,
})
```

`aylo.state.set` asks for a change; it does not happen at once. At the start of the next frame, after the input and
the network are read:

1. the `onExit` systems of the old value run;
2. the machine takes the new value;
3. the `onEnter` systems of the new value run;
4. the rest of the frame (`FixedUpdate`, `Update`...) runs in the new value.

If several systems ask in the same frame, the last one wins. Asking for the value the machine is already in cancels a
change that was asked before it. While the game is paused (in the [editor](editor.md)), nothing changes.

!!! note "Each side has its own states"
    States are not sent over the network: the server and each client declare their own machines and change them
    themselves. To make the clients follow the server's phase, put the phase in a replicated component or resource
    and have a client system call `aylo.state.set` when it changes. See [Networking](networking.md).

## Errors

All are raised when the script loads and name the system or the machine:

| Message | Cause |
|---|---|
| `System 'game.A': unknown state machine 'game.Nope' (declare it with engine.ecs.state)` | The machine is not declared, or is declared after the system. |
| `System 'game.A': state machine 'game.Phase' has no value 'Paused'` | A value that is not in the machine's list. |
| `... runs when a state changes ('onEnter' or 'onExit'): it takes no 'phase'` | An `onEnter` or `onExit` system with a `phase`. |
| `... the StateTransition phase is not written: give the system 'onEnter' or 'onExit'` | `phase = "StateTransition"`. |
| `... takes one of 'inState', 'onEnter' and 'onExit'` | Two of these options on one system. |
| `... 'inState' must be { machine, value }, such as { "game.Phase", "Playing" }` | The option is not a pair of strings. |
| `State machine 'game.Mode': its values must be a list of strings` | `engine.ecs.state` with a value that is not a string. |

Declaring a machine again with other values, with no value, or with a value twice is an error naming the machine.
`aylo.state.set` with an unknown machine or value raises a Luau error in the system that made the call.

## See also

- [Systems](systems.md): phases, queries and the other options.
- [Engine calls](engine-calls.md): how `ctx.call` works.
- [Networking](networking.md): what the server and the clients share.
