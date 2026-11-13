# The script hot path

Every piece of gameplay is a Luau system, so what a script pays per entity bounds every game: a loop over 10,000
entities runs its body 10,000 times a tick, and each step crosses into C++ to get its views and again for each field.
This page says what that crossing costs, and how the `luau` and `scripting` modules keep it low.

## Two ways to cross

The `luau` wrapper has two kinds of handlers.

| | `Value` handlers | Raw handlers |
|---|---|---|
| API | `Function::setHandler`, `Usertype::index`, `newIndex`, getters, setters, methods | `Function::setRawHandler`, `Usertype::rawIndex`, `rawNewIndex` |
| Arguments | read into `Value`s (`CallContext`) | read in place (`StackSlot`), or not at all |
| Results | a `ValuePack` of `Value`s | pushed as they are made (`Pusher`) |
| A table or a userdata | held through a registry reference (`Ref`): a `lua_ref` to make it and a `lua_unref` to drop it | goes straight to the stack |

`Value` handlers are the safe default: typed, checked, easy to test. Use a raw handler only for what a script does
once per entity or per field: the query iterator (`SystemRun.cpp`) and the views (`ViewTypes.cpp`). Both stay in
`luau`'s own types: no module outside `luau` sees a `lua_State`.

What the raw paths save:

- **The iterator** pushes the `Entity` and one `ComponentView` per term with `Pusher::userdata`, instead of a
  `ValuePack` of userdata each held through the registry, and reads none of the arguments the generic `for` passes it.
- **A field read** pushes a number or a boolean directly, a `vec2`/`vec4` as a new table (`Pusher::record`), an entity
  as a userdata. Other types (strings, asset ids, lists) go through `readField()` and a `Value`, as before.
- **A field write** reads a number or a boolean with `StackSlot::number()`/`boolean()`, and visits a vector table where
  it is (`StackSlot::forEachEntry`). Anything else, and every wrong type, goes through `writeField()`, so the values
  and the error messages are the same on both paths.
- **The metamethods** skip the getter, setter and method lookups of a type that has none (a view has none), and keep a
  raw handler's callable without a second `std::function`. A `Pusher` trusts the `LUA_MINSTACK` slots Luau guarantees
  a C function before it asks for more.

## Native code

`RuntimeConfig::nativeCode` compiles every chunk to machine code when it is loaded (`luau_codegen_compile`), on the
platforms Luau's code generator supports (x64, arm64); `Runtime::isNative()` says whether it is on. The script host
turns it on unless `project.luau` sets `scripts.native = false`. It speeds up the script's own work (arithmetic,
tables, calls between Luau functions), not the crossings above.

## Measured

10,000 entities, one script system in `FixedUpdate`, release build on an Apple M-series laptop (one run of 30 to 300
frames; nanoseconds per entity per tick):

| Loop body | Before | Raw paths | Raw paths + native code |
|---|---|---|---|
| empty (`for e, p in ctx.query do end`, one term) | 379 | 71 | 70 |
| `p.x = p.x + v.x; p.y = p.y + v.y` (two terms) | 853 | 327 | 310 |
| `m.pos = { x = m.pos.x + m.vel.x, y = ... }` (`vec2`) | 1,175 | 394 | 366 |
| the same work on plain Luau tables (no engine) | 17 | 17 | 9 |

A field access is now about 40 ns, of which about 21 ns is Luau's own metamethod call with the checks every usertype
makes (a raw `__index` that only pushes a constant costs that much). What is left is ours: the view's scope check, the
field lookup by name and the conversion.

## What would go further

- **Field lookup by atom.** Luau can give each string an atom (`lua_callbacks()->useratom`); a view could find its
  field by atom instead of comparing names. Worth a few nanoseconds per access.
- **Fewer userdata per step.** Each step makes `1 + terms` userdata for the collector. Reusing one view per term would
  change what a script that keeps a view sees, so it needs a design of its own.
- **Engine calls and spawns** still go through `Value`s (`ctx.call`, `ctx.commands:spawn`): fine for a few per tick,
  the next thing to make raw if a game calls them per entity.
