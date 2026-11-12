# Post-processing

The renderer draws your scene in a floating-point image, so a colour can be brighter than white, and only then brings it
to the window. Between the two it can make bright colours **glow** (bloom) and choose how bright colours are shown (the
**tonemap** and the **exposure**). One call sets all of it, and the default changes nothing of your picture.

Post-processing is part of `aylo.graphics`, which every game with a window already has. It lives on the **client** only.

## A first glow

```luau
-- scripts/client/look.luau: required by the client entry only
engine.call("aylo.backend.setPostProcess", {
	bloom = true,
	bloomThreshold = 0.8,
	tonemap = "aces",
})
```

Everything brighter than `0.8` now glows, and the colours go through the ACES filmic curve. Call it again at any time,
from a system too (list `"aylo.backend.setPostProcess"` in its `calls`), to change the look: a flash on a big hit, a
dimmer picture on the pause menu.

## `aylo.backend.setPostProcess`

A call **replaces every setting**: a field you leave out goes back to its default, it does not keep its last value.

| Argument | Type | Default | What it does |
|---|---|---|---|
| `bloom` | boolean | `false` | Whether bright colours glow |
| `bloomThreshold` | number | `1` | Brightness from which a colour glows; `1` is white. At least `0` |
| `bloomKnee` | number | `0.5` | Width of the soft transition under the threshold, `0` (hard edge) to `1` |
| `bloomIntensity` | number | `0.35` | How much of the glow is added back. At least `0` |
| `tonemap` | string | `"none"` | `"none"`, `"reinhard"` or `"aces"`, see below |
| `exposure` | number | `1` | Multiplies the colours before the tonemap. At least `0` |

It returns nothing. A number out of its range is brought to the closest valid value (a `bloomKnee` of `7` is `1`).

### Tonemaps

| Name | Look |
|---|---|
| `"none"` | The colours are cut at white. This is how a game looks without post-processing |
| `"reinhard"` | Soft: bright colours get close to white without reaching it |
| `"aces"` | Contrasted, filmic: bright colours reach white |

### What glows

Bloom keeps the part of a colour **above the threshold**. With the default threshold of `1`, only colours brighter than
white glow, and a normal sprite is never brighter than white. Two ways to get a glow:

- lower `bloomThreshold` (`0.7` to `0.9`) so the bright parts of your sprites glow;
- write a [shader](setup-forms.md) that outputs a colour above `1` (`COLOR.rgb *= 4.0`) for lasers and explosions, and
  keep the threshold at `1`: only they glow, and the rest of the picture is untouched.

## Recipes

A brighter, glowing picture each time the player is hit (call it again with no arguments to go back):

```luau
engine.ecs.system("game.HitFlash", {
	phase = "Update",
	side = "client",
	calls = { "aylo.backend.setPostProcess" },
	events = { read = { "game.PlayerHit" } },
	run = function(ctx)
		for _ in ctx.events.read("game.PlayerHit") do
			ctx.call("aylo.backend.setPostProcess", { bloom = true, bloomIntensity = 1.2, exposure = 1.6 })
		end
	end,
})
```

## Errors

| Message | Cause |
|---|---|
| `engine call 'aylo.backend.setPostProcess': Unknown tonemap 'filmic' (expected 'none', 'reinhard' or 'aces')` | A `tonemap` of no known name |

## Good to know

- The settings apply to the **window**. The user interface is drawn in the scene, so it gets the exposure and the tonemap
  too: keep `tonemap = "none"` and `exposure = 1` if the interface colours must stay exact.
- A scene the [editor](editor.md) draws into a panel is shown as drawn, without glow or tonemap.
- The call is absent on the server, which has no window: put it in files only the client entry requires (see
  [Systems](systems.md#where-a-system-runs)).
- A renderer that cannot do post-processing ignores the settings.

## See also

- [Setup forms](setup-forms.md): shaders and materials.
- [Engine calls](engine-calls.md): how calls are made.
- [Particles](particles.md): effects that look good with a glow.
