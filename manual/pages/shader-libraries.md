# Shader libraries

A shader can take functions from other files with `#include`: your own (a palette, a lighting trick you use in every
material) and three libraries the engine gives, for the functions procedural materials and screen effects start from:
**noise** (white, value, Perlin, simplex, fBm, Voronoi), **colour** (sRGB, HSV, luminance, blend modes) and **maths**
(remapping, rotations, random numbers from a seed).

`#include` works in every kind of shader: [sprites and materials](materials.md), [lit 3D materials](lighting.md) and
[screen effects](post-processing.md). It runs on the client, where shaders are compiled.

## A first include

A screen effect that lays drifting smoke over the picture, and tints the dark parts with a colour your game keeps in a
file of its own:

```glsl
// assets/shaders/lib/palette.glsl: our game's colours.
uniform vec3 ink = vec3(0.1, 0.05, 0.2);
vec3 inked(vec3 color, float amount) { return mix(color, ink, amount); }
```

```glsl
// assets/shaders/smoke.glsl
#pragma screen
#include "@aylo/noise.glsl"
#include "@aylo/color.glsl"
#include "lib/palette.glsl"
uniform float strength = 0.6;
void fragment() {
    float smoke = fbm(vec3(SCREEN_UV * 6.0, TIME * 0.2), 5, 0.5, 2.0) * 0.5 + 0.5;
    COLOR.rgb = blendSoftLight(COLOR.rgb, vec3(smoke), strength);
    COLOR.rgb = inked(COLOR.rgb, (1.0 - luminance(COLOR.rgb)) * 0.3);
}
```

```luau
local smoke = engine.assets.material({
	name = "game.smoke",
	shader = engine.assets.shader("shaders/smoke.glsl"),
	values = { strength = 0.4, ink = { x = 0.2, y = 0.0, z = 0.1 } },
})
engine.screenEffects({ smoke })
```

`ink` is declared in the included file, and is a value of the material like `strength`: a `uniform`, a
`uniform sampler2D` or a `varying` works the same in an included file as in the shader.

## How `#include` finds a file

| You write | It reads |
|---|---|
| `#include "@aylo/noise.glsl"` | The engine's library (`noise`, `color` or `math`) |
| `#include "lib/palette.glsl"` | First `lib/palette.glsl` next to the file that includes it (`assets/shaders/lib/palette.glsl` from `assets/shaders/smoke.glsl`), then from the assets' root (`assets/lib/palette.glsl`) |

The rules:

- The `#include` is alone on its line, the path between double quotes.
- An included file can include others, up to 16 levels deep. A file that includes itself, directly or through another,
  is an error.
- Each file is included **once** in a shader: a second `#include` of it is skipped. Two of your files can both include
  `@aylo/math.glsl` without declaring its functions twice.
- An error in an included file names that file and its line: `Shader 'shaders/smoke.glsl', line 2 of
  'shaders/lib/palette.glsl': ...`.
- With [hot reload](hot-reload.md) on, saving an included file under `assets/shaders/` compiles again every shader that
  includes it. Keep your included files there.
- The engine's libraries include nothing: include each one you use.

## `@aylo/noise.glsl`

Noise gives a value that changes smoothly with the position you give it: the same position, the same value, on every
GPU. Feed it a world position for a material that does not slide when the model moves, a `SCREEN_UV` for a screen
effect, and add `TIME` to a coordinate to make it move. Each function takes a `float`, `vec2`, `vec3` or `vec4`
position.

| Function | Result | What it gives |
|---|---|---|
| `whiteNoise(p)` | `float`, 0 to 1 | A random number for each position, unrelated to its neighbours: grain, sparkles |
| `valueNoise(p)` | `float`, 0 to 1 | Random values at whole-number positions, blended smoothly between them: soft blobs |
| `perlinNoise(p)` | `float`, about -1 to 1 | Smooth hills and valleys, 0 at every whole-number position: the classic noise |
| `simplexNoise(p)` | `float`, about -1 to 1 | Like Perlin noise, cheaper in 3D and 4D, without its square look |
| `fbm(p, octaves, gain, lacunarity)` | `float`, about -1 to 1 | Fractal Brownian motion: `octaves` layers of Perlin noise, each `gain` times as strong and `lacunarity` times as fine as the one before. Clouds, terrain, smoke: try `4` to `6` octaves, gain `0.5`, lacunarity `2.0` |
| `voronoi(p)` | `vec3` | Cells around random points (`vec2` or `vec3` position): `x` the distance to the nearest point, `y` to the second nearest, `z` = `y - x`, which is 0 on the border between two cells |
| `voronoi(p, metric, exponent)` | `vec3` | The same with another distance (below) |
| `hashBits(v)` | `uint` | Well-mixed bits of a `uint`, `uvec2`, `uvec3` or `uvec4` |
| `hashToFloat(bits)` | `float`, 0 to 1 | A number from `hashBits()` |

The distances of `voronoi`, its `metric`:

| `metric` | Cells | `exponent` |
|---|---|---|
| `VORONOI_EUCLIDEAN` | Round: the straight-line distance | unused |
| `VORONOI_MANHATTAN` | Diamond-shaped: `abs(dx) + abs(dy)` | unused |
| `VORONOI_CHEBYSHEV` | Square: `max(abs(dx), abs(dy))` | unused |
| `VORONOI_MINKOWSKI` | Between the others | `1` is Manhattan, `2` Euclidean, a large one Chebyshev |

Scale the position to choose the size of the pattern: `perlinNoise(WORLD_POSITION * 4.0)` has four hills per metre.
Bring a signed noise to 0..1 with `n * 0.5 + 0.5`.

### Cracked stone

```glsl
// assets/shaders/stone.glsl
#pragma mesh
#pragma lit
#include "@aylo/noise.glsl"
void fragment() {
    vec3 cells = voronoi(WORLD_POSITION * 3.0);
    float crack = smoothstep(0.0, 0.05, cells.z);  // dark on the borders
    float grain = fbm(WORLD_POSITION * 12.0, 4, 0.5, 2.0) * 0.5 + 0.5;
    ALBEDO = vec3(0.45, 0.43, 0.4) * mix(0.8, 1.1, grain) * mix(0.2, 1.0, crack);
    ROUGHNESS = 0.9;
}
```

## `@aylo/color.glsl`

| Function | Result | What it does |
|---|---|---|
| `srgbToLinear(c)` | same as `c` | A colour as an image stores it (sRGB) to linear, where light adds up. `float`, `vec3` or `vec4` (alpha kept) |
| `linearToSrgb(c)` | same as `c` | The other way |
| `luminance(rgb)` | `float` | How bright a linear colour looks (Rec. 709 weights) |
| `rgbToHsv(rgb)` | `vec3` | Hue (0 to 1, red at 0), saturation, value |
| `hsvToRgb(hsv)` | `vec3` | The other way; the hue wraps around, so `hsv.x + TIME` cycles the colours |

The **blend modes** lay `blend` over `base` the way a paint program does, then mix the result with `base` by
`opacity`: `0` leaves `base`, `1` is the mode's full result. Each takes `(vec3 base, vec3 blend, float opacity)` and
returns a `vec3`.

| Function | Result at `opacity = 1` |
|---|---|
| `blendMultiply` | `base * blend`: darkens |
| `blendScreen` | `1 - (1 - base) * (1 - blend)`: lightens |
| `blendOverlay` | Multiply where `base` is dark, screen where it is light: contrast |
| `blendSoftLight` | A gentle overlay (the W3C formula); a blend of `0.5` changes nothing |
| `blendLinearLight` | `base + 2 * blend - 1` |
| `blendAdd` | `base + blend`, not clamped: it can go above 1 for a glow |
| `blendSubtract` | `base - blend`, at least 0 |
| `blendDifference` | `abs(base - blend)` |
| `blendDarken`, `blendLighten` | The darker, or the lighter, of the two, per channel |
| `blendColorDodge`, `blendColorBurn` | Brightens, or darkens, `base` by `blend`, per channel |

## `@aylo/math.glsl`

| Name | Result | What it gives |
|---|---|---|
| `PI`, `TAU` | `float` | π and 2π |
| `remap(value, fromLow, fromHigh, toLow, toHigh)` | same as `value` | `value` moved from one range to another, not clamped: `remap(0.5, 0.0, 1.0, -1.0, 1.0)` is `0`. `float`, `vec2`, `vec3` or `vec4` |
| `remapClamped(value, fromLow, fromHigh, toLow, toHigh)` | `float` | The same, kept between `toLow` and `toHigh` |
| `rotate(point, angle)` | `vec2` | `point` turned by `angle` radians around the origin |
| `rotate(point, axis, angle)` | `vec3` | `point` turned by `angle` radians around `axis` |
| `random(seed)` | `float`, 0 to 1 | A number from a `uint` or `float` seed: the same seed, the same number |
| `random(seed, index)` | `float`, 0 to 1 | The `index`-th number of the sequence `seed` starts (both `uint`) |
| `randomBits(seed)` | `uint` | Well-mixed bits of a `uint` |

## Errors

| Message | Cause |
|---|---|
| `Shader 'shaders/smoke.glsl', line 4: cannot find "lib/pallete.glsl" (looked for 'shaders/lib/pallete.glsl', 'lib/pallete.glsl')` | No file at either place: a typo, or a file outside `assets/` |
| `Shader '...', line 2: the engine has no library '@aylo/noize.glsl' (@aylo/noise.glsl, @aylo/color.glsl, @aylo/math.glsl)` | A name after `@aylo/` the engine does not have |
| `Shader '...', line 1 of 'shaders/b.glsl': 'shaders/a.glsl' includes itself` | Two files that include each other |
| `Shader '...', line 1: expected #include "path"` | The path is not between double quotes |
| `Shader '...' (sprite fragment): line 2 of 'shaders/lib/palette.glsl': 'nothing' : undeclared identifier` | An error in an included file, named with its line there |
| `Shader '...': ... 'fbm' : no matching overloaded function found` | A function called with the wrong types: `fbm(p, 4.0, ...)` instead of `fbm(p, 4, ...)` (`octaves` is an `int`) |

The shader is recorded as failed and the message printed, as for any shader that does not compile; with hot reload, the
last version that compiled stays on screen.

## Good to know

- A library adds its functions to your shader; the compiler drops those you do not call, so including one costs
  nothing when the game runs.
- Your own functions must not reuse a library's names with the same parameter types.
- `engine.assets.shader("@aylo/noise.glsl")` is not a shader: libraries are only for `#include`.

## See also

- [Materials](materials.md): shader files, values and textures.
- [Lit materials](lighting.md): the surface a 3D material fills.
- [Post-processing](post-processing.md): screen effects.
- [Hot reload](hot-reload.md): changing a shader while the game runs.
