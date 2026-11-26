# Behaviours

A Behaviour is a script you attach to entities, in the style of Unity: you declare its fields, write the hooks
(`onStart`, `onUpdate`, `onFixedUpdate`) as methods, and each entity that has it runs them with its own `self`. You
need no knowledge of queries to write one. `engine.behaviour` is always there: there is nothing to turn on in
`project.luau`.

A Behaviour is a way of writing a [component](components.md) and some [systems](systems.md), not a second engine:
[What a Behaviour generates](#what-a-behaviour-generates) shows the same thing written by hand. So everything you know
about components holds: a Behaviour is saved in capsules, edited in the editor, replicated if you ask, and reloaded in
place.

```luau
-- scripts/behaviours/spinner.luau, required by an entry point
local Spinner = engine.behaviour("game.Spinner", {
	fields = { speed = "f32" },
	defaults = { speed = 3.14 }, -- radians per second: half a turn
	side = "client",
})

function Spinner:onUpdate(dt)
	local transform = self:getComponent("aylo.Transform2D")
	transform.rotation += self.speed * dt
end

return Spinner
```

Then give the component to an entity, like any other: in a [capsule](capsules.md) node
(`game.Spinner = { speed = 6 }`), or from a system:

```luau
ctx.commands:spawn({ ["game.Spinner"] = {}, ["aylo.Transform2D"] = {}, ["aylo.SpriteRenderer"] = { texture = star } })
```

Like every script, the file runs only when an entry point `require`s it, see [Modules and `require`](modules.md).

## The declaration

`engine.behaviour(name, options)` declares the component `name` and returns the Behaviour, a table you write the hooks
on. Return it from the file so other files can require it.

| Option | Meaning |
|---|---|
| `fields` | **Required.** The fields of each entity's `self`, a table of field name to [type](components.md#field-types). An empty table is allowed. |
| `defaults` | The starting value of some fields; a field not listed starts at zero. |
| `replication` | `"none"` (default), `"everyChange"` or `"spawnOnly"`: whether the server sends the fields to the clients, see [Where components live](components.md#where-components-live). |
| `presence` | `"both"` (default), `"serverOnly"` or `"clientOnly"`: which side the component exists on. |
| `side` | `"server"` (default), `"client"` or `"both"`: where the hooks run. One side for the whole Behaviour. |

Any other option is an error. The first four are those of [`engine.ecs.component`](components.md#the-declaration).

## Hooks

Write a hook as a method of the Behaviour, `function Spinner:onUpdate(dt)`. Define only the ones you need.

| Hook | When it runs, for each entity that has the Behaviour |
|---|---|
| `onStart(self)` | Once, at the start of the first frame the entity has the Behaviour (the `PreUpdate` phase), before its first other hook. |
| `onUpdate(self, dt)` | Every frame (`Update` phase). `dt` is the length of the frame, in seconds. |
| `onFixedUpdate(self, dt)` | Every fixed tick (`FixedUpdate` phase), where server gameplay goes. `dt` is the fixed step, in seconds. |

An entity that gets the Behaviour during a frame starts at the beginning of the next one: its first `onUpdate` comes
after its `onStart`, never before. An entity [switched off](systems.md#switching-an-entity-off) runs no hook, and
starts when it is switched on.

Other functions you put on the Behaviour are helpers, called on `self` like the hooks:

```luau
function Turret:isHot()
	return self.heat > 10
end

function Turret:onUpdate(dt)
	if self:isHot() then
		self.heat -= dt
	end
end
```

A name that starts with `on` and a capital letter is taken for a hook, so a typo such as `onUpdat` is an error rather
than a hook that never runs: name your helpers without that prefix.

## `self`

| You write | What it does |
|---|---|
| `self.speed`, `self.speed = 2` | Reads or writes a field of the Behaviour, on this entity. |
| `self.entity` | The entity. You can keep it in a field of type `entity`, or as a table key with `self.entity.bits`. |
| `self:getComponent(name)` | A [view](components.md#reading-and-writing) of a component of the entity, or `nil` if it has none. |
| `self:hasComponent(name)` | Whether the entity has the component. |
| `self:getParent()` | A handle on the parent of the entity, or `nil`. See [The hierarchy](systems.md#the-hierarchy). |
| `self:getChildren()` | A list of handles on its children, a new list you can keep. |
| `self:getHandle(entity)` | A handle on any entity, for example one an event gave you. |
| `self:addComponent(name, fields)` | Gives the entity the component, its defaults with `fields` over them (`ctx.commands:add`). |
| `self:removeComponent(name)` | Takes the component away (`ctx.commands:remove`). |
| `self:destroy()` | Despawns the entity (`ctx.commands:despawn`). |
| `self.ctx` | The `ctx` of the generated system: [engine calls](engine-calls.md), resources, events, time. See [`ctx`](systems.md#ctx). |

A **handle** is another entity seen the same way: it has `entity` and every method of the table above (`getComponent`,
`getParent`, `destroy`...), and no fields. Two handles on the same entity are equal (`child:getParent() == self`), and
writing a handle into a field of type `entity` writes its entity.

```luau
engine.ecs.component("game.Barrel", { fields = { cooldown = "f32" } })
local Turret = engine.behaviour("game.Turret", { fields = { heat = "f32" } })

function Turret:onFixedUpdate(dt)
	for _, barrel in self:getChildren() do
		local gun = barrel:getComponent("game.Barrel")
		if gun then
			gun.cooldown = math.max(gun.cooldown - dt, 0)
		end
	end
	if self.heat > 100 then
		self:addComponent("aylo.Disabled", {}) -- the turret and its barrels switch off
	end
end
```

Using a component through `self` needs no declaration: the generated system finds it the first time, as in
[What a system uses](systems.md#what-a-system-uses).

## What a Behaviour generates

`engine.behaviour("game.Spinner", ...)` with an `onUpdate` is exactly this:

```luau
engine.ecs.component("game.Spinner", { fields = { speed = "f32" }, defaults = { speed = 3.14 } })
engine.ecs.system("game.Spinner.onUpdate", {
	phase = "Update",
	side = "client",
	query = { write = { "game.Spinner" } },
	run = function(ctx)
		for entity, spinner in ctx.query do
			-- calls Spinner.onUpdate(self, ctx.delta), self being over (ctx, entity, spinner)
		end
	end,
})
```

- Each hook is a system named `<name>.<hook>`: `game.Spinner.onUpdate`, `game.Spinner.onFixedUpdate`,
  `game.Spinner.onStart`. Those names appear in errors and [script limits](project-settings.md#script-limits), and
  your systems can run `after` or `before` them.
- `onStart` is the system `game.Spinner.onStart`, in `PreUpdate`. It visits the entities without the tag
  `game.Spinner.Started`, gives them the tag and calls the hook; the other hooks skip an entity until it has the tag.
  The tag lives only on the side of the hooks and is never sent.
- On a client (`side = "client"` or `"both"`), a Behaviour with `replication` reads its component instead of writing
  it: the server owns those fields, so `self.speed = 2` is an error there.

Since a Behaviour is a component and systems, the rest of the engine sees nothing new: a system can query
`game.Spinner` like any component, and a [hot reload](hot-reload.md) of the file replaces the hooks in place. The
entities keep their fields, and the ones already started are not started again.

## Cost

A hook is one Luau call per entity and per frame (or tick). That is right for gameplay: players, enemies, doors,
pickups. For thousands of entities that all do the same thing, such as bullets or particles, write a
[system](systems.md), which visits them all in one call.

## Pitfalls

!!! warning "A view lives only during the hook"
    `self:getComponent` gives a view that is valid until the hook returns. Call it in each hook: keeping it in a field
    is an error (`cannot keep a view in self.transform`), and a view kept in a local of the file fails the next frame
    with `... is no longer valid`. Keep the entity instead (`self.target = other.entity`), and get its components again.

- **Changes are deferred.** `addComponent`, `removeComponent` and `destroy` happen at the end of the phase, as
  [commands](systems.md#commands) do: right after `self:destroy()`, the entity still has its components.
- **Only declared fields.** `self.cooldown = 3` on a field you did not declare is an error. The state of a Behaviour
  lives in its fields, so it is replicated, saved in capsules, reset when you stop the game and kept by hot reload; a
  plain Luau table would be none of those.
- **One side.** The hooks of a Behaviour all run on its `side`. What only a client shows (an animation, a sound) goes
  in a `side = "client"` Behaviour, and the gameplay in a server one.

## Errors

| Message | Cause |
|---|---|
| `engine.behaviour 'game.Spinner': 'fields' is required: ...` | The declaration has no `fields` table |
| `engine.behaviour 'game.Spinner': unexpected 'phase', expected 'fields', ...` | An option that does not exist |
| `engine.behaviour 'game.Spinner': the field 'getParent' has the name of a method of self: rename it` | A field named `entity`, `ctx`, like a hook or like a method of `self` |
| `engine.behaviour 'game.Spinner': 'onUpdat' is not a hook (hooks are onStart, onUpdate, onFixedUpdate); ...` | A function whose name starts with `on` and a capital letter, and is not a hook |
| `engine.behaviour 'game.Spinner': 'onUpdate' is defined twice` | The same hook written twice |
| `engine.behaviour 'game.Spinner': 'speed' is a field or a method of self, a function cannot take its name` | A helper named like a field or a method |
| `'game.Spinner' has no field 'cooldown': a Behaviour keeps its state in its declared fields, add it to 'fields'` | Writing a field that is not declared |
| `'game.Spinner' has no field 'cooldown'` | Reading a field that is not declared, and is no method or helper |
| `cannot keep a view in self.transform: a view only lives during the hook, call self:getComponent in each hook` | Writing a view into a field |
| `getComponent is a method: call it with ':', as in self:getComponent(...)` | `self.getComponent(...)` written with a `.` (any method) |
| `getComponent expects the name of a component: ...` | A name that is not a string (also `hasComponent`, `addComponent`, `removeComponent`) |
| `getHandle expects an entity: ...` | `self:getHandle` with something that is not an entity |
| `cannot write 'speed': the view of 'game.Spinner' is read-only` | A client hook writing a replicated field |
| `an entity handle has no 'speed': ...`, `cannot write 'speed' on an entity handle: ...` | A field read or written on a handle: get its component instead |

An error inside a hook is reported with the name of its system (`game.Spinner.onUpdate`), and the game goes on, as for
any [system](systems.md#errors).

## See also

- [Components](components.md) and [Systems](systems.md): what a Behaviour is made of.
- [Capsules](capsules.md): put a Behaviour on the nodes of a capsule.
- [Hot reload](hot-reload.md): editing a Behaviour while the game runs.
