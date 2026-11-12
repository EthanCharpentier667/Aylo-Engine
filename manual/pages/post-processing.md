# Post-processing

The renderer draws your scene in a floating-point image, where a colour can be brighter than white, before it reaches
the window. In between, **screen effects** can change the whole picture: make bright colours glow, grade the colours,
blur, pixelate, or turn the game into ASCII art. A screen effect is a shader you write in GLSL, in the same dialect as the
shaders of your sprites, and the engine gives you two ready-made ones: a **bloom** and a **tonemap**.

Screen effects come with the `aylo.Shader` plugin, which every game with a window already has. They run on the
**client** only.

## A first glow

```luau
-- In a script of the client (scripts/client/...), or of both sides: a server ignores it.
local bloom = engine.assets.material({
	name = "game.bloom",
	shader = engine.assets.shader("@aylo/bloom.glsl"),
	values = { threshold = 0.8, intensity = 0.5 },
})
local tonemap = engine.assets.material({
	name = "game.tonemap",
	shader = engine.assets.shader("@aylo/tonemap.glsl"),
})
engine.screenEffects({ bloom, tonemap })
```

Everything brighter than `0.8` now glows, then the colours go through the ACES filmic curve. The effects run in the
order of the list, each one reading what the previous one wrote.

## Choosing the effects

| Form or call | Arguments | What it does |
|---|---|---|
| `engine.screenEffects(materials)` | a list of material ids | Sets the screen effects, in their order, replacing the previous ones. `{}` removes them all. Does nothing on a server |
| `aylo.shader.setScreenMaterials` | `materials`: a list of material ids | The call behind the form, for a system (list it in its `calls`). Absent on a server |

Both return nothing. Without effects, the default, your scene is shown as drawn.

A material in the list whose shader did not compile, or whose shader is not a screen shader, is skipped: the other
effects still run.

## The engine's effects

Load them with `engine.assets.shader("@aylo/<name>.glsl")`, then give their values in the material, as for any shader.

### `@aylo/bloom.glsl`

Makes what is brighter than a threshold glow around itself.

| Value | Type | Default | What it does |
|---|---|---|---|
| `threshold` | `float` | `1` | Brightness from which a colour glows; `1` is white |
| `softKnee` | `float` | `0.5` | `0`: a hard threshold; up to `1`: the glow fades in below it |
| `intensity` | `float` | `0.35` | How much of the glow is added to the picture |
| `radius` | `float` | `24` | How far the glow reaches, in pixels |

With the default threshold of `1`, only colours brighter than white glow, and a normal sprite is never brighter than
white. You get a glow in either of two ways:

- lower `threshold` (`0.7` to `0.9`) so the bright parts of your sprites glow;
- give lasers and explosions a shader that writes a colour above `1` (`COLOR.rgb *= 4.0;`) and keep the threshold at
  `1`, so only they glow.

### `@aylo/tonemap.glsl`

Brings the floating-point picture, where a colour may exceed 1, back to what a screen shows. Put it after the effects
that make colours brighter (the bloom), since it brings every colour back between 0 and 1.

| Value | Type | Default | What it does |
|---|---|---|---|
| `exposure` | `float` | `1` | Multiplies the colours first: above `1` brighter, below `1` darker |
| `mode` | `int` | `2` | `0`: none, the colours are cut at white; `1`: Reinhard, soft, bright colours get close to white; `2`: ACES, contrasted and filmic |

Both are ordinary shader files. To change one, copy it into your game and load your copy instead.

## Writing a screen effect

A screen effect is a shader file that starts with the line `#pragma screen` and defines `void fragment()`. It runs once
for each pixel of the screen.

```glsl
// assets/shaders/grayscale.glsl
#pragma screen

uniform float amount = 1.0;  // 0: the colours as they are, 1: all gray.

void fragment() {
    float gray = dot(COLOR.rgb, vec3(0.299, 0.587, 0.114));
    COLOR.rgb = mix(COLOR.rgb, vec3(gray), amount);
}
```

```luau
local gray = engine.assets.material({
	name = "game.gray",
	shader = engine.assets.shader("shaders/grayscale.glsl"),
	values = { amount = 0.8 },
})
engine.screenEffects({ gray })
```

What a screen effect can read and write:

| Name | Type | What it is |
|---|---|---|
| `COLOR` | `vec4` | Starts as this pixel of the picture so far. What you leave in it is written as is: no blending |
| `SCREEN_TEXTURE` | `sampler2D` | The picture so far: the scene for the first effect, then what the previous effect wrote. `TEXTURE` is the same |
| `SCREEN_UV` | `vec2` | Where this pixel is on the screen, from `(0, 0)` at the top-left corner to `(1, 1)` at the bottom-right. `UV` is the same |
| `SCREEN_PIXEL_SIZE` | `vec2` | The size of one pixel in `SCREEN_UV` units: `1 / the screen's size in pixels` |
| `TIME` | `float` | Seconds since the renderer started |

The colours are in floating point: a colour may be above `1` before a tonemap brings it back. Read other pixels with
`texture(SCREEN_TEXTURE, SCREEN_UV + offset)`, where one pixel to the right is `vec2(SCREEN_PIXEL_SIZE.x, 0.0)`.

Declare values with `uniform <type> <name> = <default>;`, one per line, outside any function. The types are `float`,
`vec2`, `vec3`, `vec4` and `int`; set them in `values` of the material, as a number or a vector `{x, y}`, `{x, y, z}` or
`{x, y, z, w}`. A
`uniform sampler2D <name>;` is an extra texture you give in `textures`, for a font of glyphs or a noise. An
`instance uniform` keeps its default in a screen effect, since no entity holds its own values: use a `uniform`.

The names above, and those starting with `aylo`, are the engine's: you cannot declare them. `SCREEN_TEXTURE`,
`SCREEN_UV` and `SCREEN_PIXEL_SIZE` only exist in a screen effect, and a sprite whose material is a screen effect is
drawn as if it had none.

A shader file under `assets/shaders/` is compiled again when you save it, in a game with [hot reload](hot-reload.md) on.
A screen effect follows: you see the change on the next frame, and a typo keeps the last version on screen.

## Recipes

### The whole game in ASCII art

Each cell of 8 by 8 pixels becomes a dot as big as the cell is bright, in green, like an old terminal. Swap the dot for
glyphs from a texture to get real letters.

```glsl
// assets/shaders/ascii.glsl
#pragma screen

uniform float cellSize = 8.0;  // In pixels.

void fragment() {
    vec2 cell = cellSize * SCREEN_PIXEL_SIZE;
    vec2 center = (floor(SCREEN_UV / cell) + 0.5) * cell;
    float brightness = dot(texture(SCREEN_TEXTURE, center).rgb, vec3(0.299, 0.587, 0.114));
    float mark = step(length(fract(SCREEN_UV / cell) - 0.5), 0.5 * clamp(brightness, 0.0, 1.0));
    COLOR = vec4(vec3(0.2, 1.0, 0.3) * mark, 1.0);
}
```

```luau
local ascii = engine.assets.material({ name = "game.ascii", shader = engine.assets.shader("shaders/ascii.glsl") })
engine.screenEffects({ ascii })
```

### A stronger glow while the player is hit

The effects can change at any time, from a system:

```luau
local calm = { bloom, tonemap }
local hit = engine.assets.material({
	name = "game.hitBloom",
	shader = engine.assets.shader("@aylo/bloom.glsl"),
	values = { threshold = 0.5, intensity = 1.2 },
})

engine.ecs.system("game.HitFlash", {
	phase = "Update",
	side = "client",
	calls = { "aylo.shader.setScreenMaterials" },
	events = { read = { "game.PlayerHit" } },
	run = function(ctx)
		for _ in ctx.events.read("game.PlayerHit") do
			ctx.call("aylo.shader.setScreenMaterials", { materials = { hit, tonemap } })
		end
	end,
})
```

Here `bloom` and `tonemap` are the materials of [A first glow](#a-first-glow). Go back to `calm` the same way once the
hit is over.

## Errors

| Message | Cause |
|---|---|
| `engine.screenEffects: expected a list of materials` | The argument is not a table |
| `engine.screenEffects: engine call 'aylo.shader.setScreenMaterials': No material has the id <id>` | An id in the list is not a material: a typo, or a shader id given instead of its material |
| `[ShaderAssets] Shader '@aylo/vignette.glsl': the engine has no such shader (@aylo/bloom.glsl, @aylo/tonemap.glsl)` | A name after `@aylo/` the engine does not have (printed, the shader is recorded as failed) |
| `[ShaderAssets] Shader 'shaders/x.glsl' (sprite fragment): line 3: 'SCREEN_UV' : undeclared identifier` | A shader that uses the screen names without the `#pragma screen` line |

## Good to know

- The [user interface](ui.md) is drawn after the effects, so they leave it alone. A text or a sprite of your world is
  part of the scene, and gets them.
- A scene the [editor](editor.md) draws into a panel gets the effects too.
- Each effect reads and writes the whole screen once, so a long list costs more on a slow GPU. The engine's bloom reads
  64 pixels for each pixel.
- A renderer that cannot run game shaders shows the scene as drawn.

## See also

- [Setup forms](setup-forms.md): `engine.assets.shader` and `engine.assets.material`.
- [Engine calls](engine-calls.md): how calls are made from a system.
- [Hot reload](hot-reload.md): changing a shader while the game runs.
- [Particles](particles.md): effects that look good with a glow.
