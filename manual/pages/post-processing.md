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
| `engine.screenEffects(materials)` | a list of material ids, or a list of passes ([Several passes](#several-passes)) | Sets the screen effects, in their order, replacing the previous ones. `{}` removes them all. Does nothing on a server |
| `aylo.shader.setScreenMaterials` | `materials`: a list of material ids | The call behind the form, for a system (list it in its `calls`). Replaces the effects, passes included. Absent on a server |
| `aylo.shader.addScreenEffect` | `material`; `input`, `output`: strings (default `""`); `scale`: number (default `1`); `width`, `height`: whole numbers (default `0`: by `scale`); `lowPrecision`: boolean (default `false`) | Adds a pass after the others. Call `setScreenMaterials` with `{}` first to start over. Absent on a server |
| `aylo.shader.addScreenEffectInput` | `sampler`: a `uniform sampler2D` of the last pass's shader; `target`: a target's name | Binds that texture of the last pass added to a target. Absent on a server |
| `aylo.shader.checkScreenEffects` | none | Fails, and removes the passes, if one reads a target no pass before it writes. Absent on a server |

They return nothing. Without effects, the default, your scene is shown as drawn.

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
| `SCREEN_TEXTURE` | `sampler2D` | The picture so far: the scene for the first effect, then what the previous effect wrote (a pass with an `input` reads that target instead). `TEXTURE` is the same |
| `SCENE_TEXTURE` | `sampler2D` | The scene as drawn, before any effect |
| `PREVIOUS_FRAME` | `sampler2D` | The last frame once every effect ran, before the interface; black on the first frame. Kept only while an effect reads it |
| `SCREEN_UV` | `vec2` | Where this pixel is on the screen, from `(0, 0)` at the top-left corner to `(1, 1)` at the bottom-right. `UV` is the same |
| `SCREEN_PIXEL_SIZE` | `vec2` | The size of one pixel in `SCREEN_UV` units: `1 / the size in pixels of what this effect writes` (the screen, or a smaller target) |
| `TIME` | `float` | Seconds since the renderer started |

The colours are in floating point: a colour may be above `1` before a tonemap brings it back. Read other pixels with
`texture(SCREEN_TEXTURE, SCREEN_UV + offset)`, where one pixel to the right is `vec2(SCREEN_PIXEL_SIZE.x, 0.0)`.

Declare values with `uniform <type> <name> = <default>;`, one per line, outside any function. The types are `float`,
`vec2`, `vec3`, `vec4` and `int`; set them in `values` of the material, as a number or a vector `{x, y}`, `{x, y, z}` or
`{x, y, z, w}`. A
`uniform sampler2D <name>;` is an extra texture you give in `textures`, for a font of glyphs or a noise. An
`instance uniform` keeps its default in a screen effect, since no entity holds its own values: use a `uniform`.

The names above (and those of [Depth and the 3D camera](#depth-and-the-3d-camera)), and those starting with `aylo`, are the engine's: you cannot declare them. `SCREEN_TEXTURE`,
`SCREEN_UV` and `SCREEN_PIXEL_SIZE` only exist in a screen effect, and a sprite whose material is a screen effect is
drawn as if it had none.

## Depth and the 3D camera

A screen effect can read how far each pixel of the 3D world is, and where the 3D camera is: fog, a depth of field,
outlines found at the edges of things, a scan line that sweeps the ground.

```glsl
// assets/shaders/fog.glsl
#pragma screen
uniform vec3 fogColor = vec3(0.5, 0.6, 0.7);
uniform float density = 0.04;

void fragment() {
    float distance = linearDepth(texture(DEPTH_TEXTURE, SCREEN_UV).r);  // metres from the camera
    COLOR.rgb = mix(fogColor, COLOR.rgb, exp(-density * distance));
}
```

| Name | Type | What it is |
|---|---|---|
| `DEPTH_TEXTURE` | `sampler2D` | The 3D world's depth: `0` at the camera's near plane, `1` at its far plane and where nothing 3D was drawn (the 2D world, the sky) |
| `linearDepth(depth)` | `float` | A depth of `DEPTH_TEXTURE` as metres from the camera |
| `viewPositionAt(uv)` | `vec3` | The point seen at `uv`, in the camera's space (it looks along -Z) |
| `worldPositionAt(uv)` | `vec3` | The point seen at `uv`, in the world |
| `normalAt(uv)` | `vec3` | The surface's normal at `uv`, in the camera's space, toward the camera |
| `CAMERA_POSITION` | `vec3` | The 3D camera, in the world |
| `VIEW_MATRIX`, `INV_VIEW_MATRIX` | `mat4` | The world's space to the camera's, and back |
| `PROJECTION_MATRIX`, `INV_PROJECTION_MATRIX` | `mat4` | The camera's space to the screen's, and back |

The depth only covers the 3D world: the 2D world and the user interface, drawn over it, are at the far plane, so a fog
leaves them as they are. The depth is kept for the effects only when one of them reads it: a game whose effects do not
pays nothing. `normalAt()` rebuilds the normal from the depth of the neighbouring pixels, so it is the shape of what
was drawn, not a normal map's. With a `virtualResolution` that puts bars around the picture, the positions assume the 3D
view fills the screen.

## Several passes

Some effects need more than one step: a glow is blurred in a smaller image, then added back; a blur is made of a
horizontal pass, then a vertical one. Give `engine.screenEffects` a list of **passes** instead of materials. A pass is a
table: its material, and where it reads and writes.

```glsl
// assets/shaders/bright.glsl: keeps what is brighter than the threshold
#pragma screen
uniform float threshold = 0.8;
void fragment() { COLOR.rgb = max(COLOR.rgb - threshold, 0.0); }
```

```glsl
// assets/shaders/blur.glsl: averages three pixels along `direction`
#pragma screen
uniform vec2 direction = vec2(1.0, 0.0);
void fragment() {
    vec2 step = direction * SCREEN_PIXEL_SIZE;
    COLOR.rgb = (texture(TEXTURE, SCREEN_UV - step).rgb + COLOR.rgb + texture(TEXTURE, SCREEN_UV + step).rgb) / 3.0;
}
```

```glsl
// assets/shaders/composite.glsl: the scene, plus the blurred glow
#pragma screen
uniform sampler2D glow;          // bound to a target by the pass
uniform float strength = 0.4;
void fragment() { COLOR.rgb = texture(SCENE_TEXTURE, SCREEN_UV).rgb + texture(glow, SCREEN_UV).rgb * strength; }
```

```luau
local function material(name, shader, values)
	return engine.assets.material({ name = name, shader = engine.assets.shader(shader), values = values })
end
local brightPass = material("game.bright", "shaders/bright.glsl")
local blurX = material("game.blurX", "shaders/blur.glsl", { direction = { x = 1, y = 0 } })
local blurY = material("game.blurY", "shaders/blur.glsl", { direction = { x = 0, y = 1 } })
local composite = material("game.composite", "shaders/composite.glsl")

engine.screenEffects({
	{ material = brightPass, output = "bright", scale = 0.5 },
	{ material = blurX, input = "bright", output = "blurA", scale = 0.5 },
	{ material = blurY, input = "blurA", output = "blurB", scale = 0.5 },
	{ material = composite, inputs = { glow = "blurB" } },
})
```

The first three passes write **targets**, images named by their `output`, at half the screen's size: four times fewer
pixels to blur. The last pass has no `output`, so it writes the picture shown, and reads the last blur as its `glow`.

| Key | Type | Default | What it does |
|---|---|---|---|
| `material` | material id | required | The material of a `#pragma screen` shader |
| `input` | string | the picture so far | What the pass reads as `TEXTURE` (and starts `COLOR` from): a target an earlier pass wrote, or `"scene"` for the scene before any effect |
| `inputs` | table `{ sampler = target }` | none | Binds each `uniform sampler2D` of the shader, by name, to a target or `"scene"`, in place of the material's texture |
| `output` | string | none | The target the pass writes. Without it, the pass writes the picture shown, which the next pass without `input` reads |
| `scale` | number | `1` | The target's size as a share of the screen's: `0.5` is half its width and half its height |
| `size` | `{ x = width, y = height }` | none | The target's size in pixels, in place of `scale` |
| `lowPrecision` | boolean | `false` | An 8-bit target: half the memory, but colours are cut between 0 and 1. Leave it off for what can be brighter than white |

The rules:

- A pass reads only what an earlier pass wrote. A pass that reads a later target, or its own, is an error, and no pass
  of the list is kept.
- The passes without `output` form the chain you know from a list of materials: each reads what the previous one wrote.
  The picture shown is what the last of them wrote, or the scene if none.
- Targets are made when a pass first writes them, made again when the window changes size, and freed when no pass names
  them any more.
- A list of materials still works: it is the same as passes with only a `material`.

### Trails from the last frame

`PREVIOUS_FRAME` holds what the screen showed one frame ago. Mixing it in leaves a fading trail behind anything that
moves:

```glsl
// assets/shaders/trails.glsl
#pragma screen
uniform float keep = 0.9;  // How much of the last frame stays: closer to 1, longer trails.
void fragment() {
    vec3 last = texture(PREVIOUS_FRAME, SCREEN_UV).rgb;
    COLOR.rgb = max(COLOR.rgb, last * keep);
}
```

```luau
local trails = engine.assets.material({ name = "game.trails", shader = engine.assets.shader("shaders/trails.glsl") })
engine.screenEffects({ trails })
```

The last frame is kept only while an effect reads it. It is taken before the [user interface](ui.md) is drawn, so the
interface leaves no trail.

## Hot reload

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
| `engine.screenEffects: engine call 'aylo.shader.checkScreenEffects': The screen effects cannot run: pass 2 reads 'blur', which no pass before it writes` | A pass reads a target that only a later pass writes, or a typo in a target's name |
| `engine.screenEffects: pass 1: expected a 'material'` | A pass without its `material` |
| `engine.screenEffects: pass 1: unexpected 'scales', expected 'material', 'input', 'inputs', 'output', 'scale', 'size', 'lowPrecision'` | A key a pass does not have |
| `[ShaderAssets] Shader '@aylo/vignette.glsl': the engine has no such shader (@aylo/bloom.glsl, @aylo/tonemap.glsl)` | A name after `@aylo/` the engine does not have (printed, the shader is recorded as failed) |
| `[ShaderAssets] Shader 'shaders/x.glsl' (sprite fragment): line 3: 'SCREEN_UV' : undeclared identifier` | A shader that uses the screen names without the `#pragma screen` line |

## Good to know

- The [user interface](ui.md) is drawn after the effects, so they leave it alone. A text or a sprite of your world is
  part of the scene, and gets them.
- A scene the [editor](editor.md) draws into a panel gets the effects too.
- Each effect reads and writes the whole screen once, so a long list costs more on a slow GPU. The engine's bloom reads
  64 pixels for each pixel. A pass with `scale = 0.5` writes a quarter of the pixels.
- A sampler named in `inputs` that the shader does not declare is ignored.
- A renderer that cannot run game shaders shows the scene as drawn.

## See also

- [Setup forms](setup-forms.md): `engine.assets.shader` and `engine.assets.material`.
- [Materials](materials.md): keep a material in a `.material` file, edit it in the editor.
- [Shader libraries](shader-libraries.md): `#include` noise, colour and maths functions in an effect.
- [Engine calls](engine-calls.md): how calls are made from a system.
- [Hot reload](hot-reload.md): changing a shader while the game runs.
- [Particles](particles.md): effects that look good with a glow.
