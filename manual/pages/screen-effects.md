# Standard screen effects

The engine gives a set of ready-made [screen effects](post-processing.md): a wide bloom, a painterly filter, sharpening,
lens and film looks, antialiasing, ambient occlusion and fog. You turn them on from a script with `engine.effects`,
without writing a shader, and you can stack them in any order. Each one is an ordinary shader file of the engine, which
you can copy into your game to change.

They run on the **client** only, like every screen effect. A server ignores them, so a script of both sides can set them.

## A first look

```luau
-- In a script of the client (scripts/client/...), or of both sides: a server ignores it.
engine.screenEffects({
	engine.effects.ssao({ radius = 0.4 }),
	engine.effects.bloom({ threshold = 0.9, intensity = 0.5 }),
	engine.effects.kuwahara({ radius = 6, anisotropic = true }),
	"@aylo/chromatic_aberration.glsl",
	engine.effects.vignette(),
	engine.effects.tonemap({ mode = 2 }),
	engine.effects.fxaa(),
})
```

Each `engine.effects.<name>(values)` takes the effect's values, all optional, and returns what `engine.screenEffects`
needs: one pass, or several for the effects that work on smaller images. A shader's name as a string
(`"@aylo/vignette.glsl"`) is the effect with its default values. The list mixes them freely with your own materials and
passes ([Several passes](post-processing.md#several-passes)).

Each call makes its own materials, so the same effect can appear twice with different values. To change the values
while the game runs, call `engine.screenEffects` again with new ones.

## The order

The effects run in the order of the list. A good order:

1. What works on the 3D world: `ssao`, `fog`.
2. What adds light: `bloom`.
3. The look: `kuwahara`, `sharpen`, `chromaticAberration`, `filmGrain`, `vignette`.
4. `tonemap`, which brings the colours back between 0 and 1.
5. `fxaa`, which expects colours between 0 and 1.

## Bloom

Makes what is brighter than a threshold glow around itself. The picture is halved several times, each half blurred,
then added back up: the glow can reach far for a small cost.

```luau
engine.screenEffects({ engine.effects.bloom({ threshold = 0.8, intensity = 0.6, radius = 48 }), engine.effects.tonemap() })
```

| Value | Type | Default | What it does |
|---|---|---|---|
| `threshold` | number | `1` | Brightness from which a colour glows; `1` is white |
| `softKnee` | number | `0.5` | `0`: a hard threshold; up to `1`: the glow fades in below it |
| `intensity` | number | `0.35` | How much of the glow is added |
| `radius` | number | `24` | How far the glow reaches, in pixels. It chooses how many times the picture is halved, from 1 to 8 |

It has the values of the single-pass `@aylo/bloom.glsl` ([The engine's effects](post-processing.md#the-engines-effects)),
which still works. The single-pass bloom reads 64 pixels for each pixel whatever the radius. This one reads about 6 for
each pixel of the screen at the default radius, and few more at a larger one: prefer it.

## Kuwahara

Flattens the picture into patches of even colour while keeping the edges: the picture looks painted.

```luau
engine.effects.kuwahara({ radius = 4 })                          -- the classic filter: square patches
engine.effects.kuwahara({ radius = 6, anisotropic = true })      -- strokes that follow the shapes
```

| Value | Type | Default | What it does |
|---|---|---|---|
| `radius` | number | `4` (classic), `6` (anisotropic) | The size of the patches, in pixels. A whole number for the classic filter |
| `anisotropic` | boolean | `false` | `true`: the patches stretch along the picture's edges like brush strokes (Kyprianidis et al.), four passes instead of one |
| `sharpness` | number | `8` | Anisotropic: how crisp the edges between patches are |
| `eccentricity` | number | `1` | Anisotropic: how much the strokes stretch; lower stretches more |
| `uniformity` | number | `2` | Anisotropic: how smooth the strokes' directions are, the blur of the picture's structure in pixels |

Both read many pixels for each one: (radius + 1)² × 4 for the classic filter, about π × radius² × 2 for the anisotropic
one. Keep the radius small at high resolutions.

## Sharpen

Brings out edges and fine detail, for a picture that looks soft (after an upscale, or a blur).

| Value | Type | Default | What it does |
|---|---|---|---|
| `amount` | number | `0.5` | `0`: unchanged; `1` and more: strong. Too much rings around edges |
| `mode` | number | `0` | `0`: box, compares with the eight neighbours; `1`: diamond, with the four that share a side, softer |

## Chromatic aberration

Red, green and blue land a little apart, as through a cheap lens, more toward the edges of the screen.

| Value | Type | Default | What it does |
|---|---|---|---|
| `strength` | number | `0.004` | The split from the centre: red is scaled up and blue down by it |
| `offset` | `{ x, y }` | `{ x = 0, y = 0 }` | A split in pixels, the same everywhere: red moves one way, blue the other |

## Film grain

A new random grain each frame, in brightness (film) and in colour (a digital sensor in the dark), stronger in the
shadows.

| Value | Type | Default | What it does |
|---|---|---|---|
| `luminanceNoise` | number | `0.06` | How strong the grain in brightness is |
| `chromaNoise` | number | `0.02` | How strong the grain in colour is |
| `size` | number | `1.5` | A grain's size, in pixels |
| `speed` | number | `24` | New grains per second |

## Vignette

The corners of the picture fade toward a colour, which draws the eye to the centre.

| Value | Type | Default | What it does |
|---|---|---|---|
| `intensity` | number | `0.4` | `0`: none; `1`: the corners are fully `color` |
| `radius` | number | `0.75` | Where the fade starts, from the centre: `0.5` reaches the middle of the top and bottom edges |
| `softness` | number | `0.45` | How wide the fade is |
| `color` | `{ x, y, z }` | black | What the corners fade to: `{ x = 0.3, y = 0, z = 0 }` for a red pulse when hurt |

## Tonemap

`engine.effects.tonemap(values)` is `@aylo/tonemap.glsl`: `exposure` and `mode`
([The engine's effects](post-processing.md#the-engines-effects)).

## FXAA

Fast approximate antialiasing: finds the edges by their brightness and blurs along them, which smooths the stairs of
every edge, 2D and 3D, for the cost of one pass. Put it after `tonemap`.

| Value | Type | Default | What it does |
|---|---|---|---|
| `spanMax` | number | `8` | How far along an edge it may blur, in pixels |
| `reduceMul` | number | `0.125` | How much an edge's own contrast shortens the blur: lower blurs more |

## SSAO

Screen-space ambient occlusion: the 3D world gets darker in its creases, corners and under objects, where little light
from the sky reaches. It reads the [depth of the 3D world](post-processing.md#depth-and-the-3d-camera), at half the
screen's size, then blurs the result and lays it over the picture.

| Value | Type | Default | What it does |
|---|---|---|---|
| `radius` | number | `0.5` | How far around each point it looks, in world units: the size of the creases it darkens |
| `intensity` | number | `1` | How dark the hidden corners get |
| `bias` | number | `0.025` | A depth margin, in world units: raise it if flat surfaces show a dark pattern |
| `samples` | number | `16` | Points tested per pixel: more, smoother and slower |
| `blur` | number | `2` | How much the result is blurred, in pixels of the half-size image |
| `shadow` | `{ x, y, z }` | black | The colour the hidden corners take |

Only the 3D world gets it: the 2D world and the sky, where no 3D model was drawn, are left as they are. Put it first,
before the effects that change the colours.

## Fog

The 3D world fades into a colour with its distance from the camera, and can thin out with height, for a mist that lies
on the ground.

```luau
engine.effects.fog({ color = { x = 0.7, y = 0.75, z = 0.8 }, density = 0.05, height = 0, heightFalloff = 0.4 })
```

| Value | Type | Default | What it does |
|---|---|---|---|
| `color` | `{ x, y, z }` | `{ x = 0.6, y = 0.65, z = 0.7 }` | The fog's colour |
| `density` | number | `0.03` | How thick it is: at `0.69 / density` world units, half of what you see is fog |
| `start` | number | `0` | The distance where it starts, in world units |
| `height` | number | `0` | The height below which it is thickest |
| `heightFalloff` | number | `0` | How fast it thins above `height`: `0` is the same at every height |

Like SSAO, only the 3D world gets it.

## Errors

| Message | Cause |
|---|---|
| `engine.effects.bloom: unexpected 'thresold', expected 'threshold', 'softKnee', 'intensity', 'radius'` | A value the effect does not have: a typo |
| `engine.effects.kuwahara: 'anisotropic' must be a boolean` | `anisotropic = 1` instead of `true` |
| `engine.assets.material: value 'color': expected a number, or a vector {x, y}, {x, y, z} or {x, y, z, w}` | A colour or offset written `{ 1, 0, 0 }` instead of `{ x = 1, y = 0, z = 0 }` |

## Good to know

- Each effect is a shader file of the engine, under the names in the table below. To change one, copy it into your
  game's `assets/shaders/` and use your copy as a material or a pass.
- Every effect costs time on the GPU for each pixel of the screen. On a slow GPU, the anisotropic Kuwahara and SSAO
  cost the most; FXAA, the vignette, the grain and the chromatic aberration the least.

| Effect | Shaders |
|---|---|
| `bloom` | `@aylo/bloom_prefilter.glsl`, `@aylo/bloom_down.glsl`, `@aylo/bloom_up.glsl`, `@aylo/bloom_composite.glsl` |
| `kuwahara` | `@aylo/kuwahara.glsl`; anisotropic: `@aylo/kuwahara_tensor.glsl`, `@aylo/blur.glsl`, `@aylo/kuwahara_anisotropic.glsl` |
| `ssao` | `@aylo/ssao.glsl`, `@aylo/blur.glsl`, `@aylo/ssao_apply.glsl` |
| the others | `@aylo/sharpen.glsl`, `@aylo/chromatic_aberration.glsl`, `@aylo/film_grain.glsl`, `@aylo/vignette.glsl`, `@aylo/tonemap.glsl`, `@aylo/fxaa.glsl`, `@aylo/fog.glsl` |

## See also

- [Post-processing](post-processing.md): writing your own screen effects, passes, depth.
- [Shader libraries](shader-libraries.md): the noise and colour functions the effects use.
- [Lighting](lighting.md): the 3D world the fog and SSAO work on.
