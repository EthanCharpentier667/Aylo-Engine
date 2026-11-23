# Lights

Lights shine on the 3D world: the sun, a lamp, a torch, a window. You add them as components of 3D entities, and the
engine hands them to the renderer every frame, placed and turned with their entity. A [lit material](lighting.md) shades its
surface from them, and a model with no material is lit by them too; an unlit `#pragma mesh` shader ignores them. A
game without lights draws as before.

Lights come with the graphics plugin: there is nothing to add to `project.luau`.

```luau
engine.ecs.system("game.Lights", {
	phase = "Startup",
	side = "client",
	run = function(ctx)
		-- a warm window, 5 by 5 metres, on the wall at x = 3
		ctx.commands:spawn({
			["aylo.Transform3D"] = { position = vector.create(3, 2.4, 0.3) },
			["aylo.Light3D"] = { kind = "area", color = vector.create(1, 0.6, 0.38), power = 600, size = { x = 5, y = 5 } },
		})
		-- the sun, turned 50 degrees down
		ctx.commands:spawn({
			["aylo.Transform3D"] = { rotation = engine.math.quat.fromEuler(math.rad(-50), 0, 0) },
			["aylo.Light3D"] = { kind = "directional", intensity = 1.4 },
		})
		-- what lights the rest a little
		ctx.commands:spawn({ ["aylo.Environment3D"] = { ambient = vector.create(0.05, 0.05, 0.06) } })
	end,
})
```

## `aylo.Light3D`

A light is placed by its entity's `aylo.Transform3D`, and shines along the entity's **-Z**, the way a camera looks:
turn the entity to aim it. Its scale is ignored.

| Field | Type | Default | Meaning |
|---|---|---|---|
| `kind` | `"directional"`, `"point"`, `"spot"` or `"area"` | `"point"` | its shape: see below |
| `color` | vec3 | `1, 1, 1` | linear RGB |
| `power` | f32 | `1000` | point, spot and area lights: watts, as in Blender |
| `intensity` | f32 | `1` | directional lights: lux (W/m²), as Blender's sun strength |
| `range` | f32 | `0` | metres past which it lights nothing; `0`: no limit |
| `innerConeAngle` | f32 | `0` | spot: the half-angle lit fully, in degrees |
| `outerConeAngle` | f32 | `45` | spot: the half-angle past which nothing is lit, in degrees (at most 89.9) |
| `size` | vec2 | `1, 1` | area: its width (along the entity's X) and height (along Y), in metres |
| `castShadows` | bool | `false` | whether it casts shadows: see [Shadows](#shadows) |
| `enabled` | bool | `true` | `false` turns it off without removing it |
| `shadowSoftness` | f32 | `1` | `0`: hard shadows; `1`: as soft as a light of its size; more: softer |
| `shadowBias` | f32 | `1` | raise it if a lit surface shows dark stripes (shadow acne) |
| `shadowNormalBias` | f32 | `1` | the same, along the surface: raise it for stripes on slopes |

| Kind | What it is | Where it is | Strength |
|---|---|---|---|
| `"directional"` | the sun: the same light everywhere, from infinitely far | only its rotation counts | `intensity`, lux |
| `"point"` | a bulb: from a point, all around | its position | `power`, watts |
| `"spot"` | a torch: from a point, in a cone along -Z | its position and rotation | `power`, watts |
| `"area"` | a window, a screen: from a rectangle facing -Z | its position, rotation and `size` | `power`, watts |

The units are those of glTF files and Blender, so a scene lit in Blender keeps its balance in the engine: a 60 W bulb
next to a 1000 W one looks as it did there. An everyday room is a few hundred watts of lights and an ambient near
`0.03`; daylight is a sun of a few lux.

A frame draws with 64 lights at most. Past that, the engine keeps the directional lights, then the lights nearest to
the camera, so a big level can hold many lamps as long as fewer than 64 are near.

## `aylo.Environment3D`

The light that reaches every surface, wherever it is: what no light reaches is not black. Put it on any entity (it
needs no transform); with several, the one with the highest `priority` is used.

| Field | Type | Default | Meaning |
|---|---|---|---|
| `ambient` | vec3 | `0, 0, 0` | linear RGB |
| `strength` | f32 | `1` | multiplies `ambient` |
| `priority` | i32 | `0` | among several, the highest one is used |
| `shadowCascades` | u32 | `3` | directional lights: how many shadow maps share the view, 1 to 4 (more: sharper far away) |
| `shadowResolution` | u32 | `1024` | texels of a side of one shadow map: `512`, `1024` or `2048` |
| `shadowDistance` | f32 | `60` | directional lights: how far from the camera shadows fall, in metres |
| `skyTexture` | asset id | none | a sky image ([The sky](#the-sky)) |
| `skyGradient` | bool | `false` | without `skyTexture`: a sky of three colours |
| `skyTop`, `skyHorizon`, `skyBottom` | vec3 | blue, pale blue, brown | the gradient's colours straight up, at the horizon and straight down, linear RGB |
| `skyIntensity` | f32 | `1` | multiplies the sky, as drawn and as light |
| `skyRotation` | f32 | `0` | turns the sky around the vertical axis, in radians |
| `lightingFromSky` | bool | `true` | whether the sky lights the world, added to `ambient` |

## The sky

A sky is drawn behind the 3D world, where no model is, and lights it: a surface takes the sky's colour from the
directions it faces, and a shiny one reflects it, blurred as its `ROUGHNESS` grows. A white matte ball under a blue sky
turns blue-grey; a chrome ball shows the sky; metals stop looking black.

```luau
-- A gradient: no image needed.
ctx.commands:spawn({
	["aylo.Environment3D"] = {
		skyGradient = true,
		skyTop = vector.create(0.2, 0.4, 1.0),
		skyHorizon = vector.create(0.9, 0.8, 0.7),
		skyIntensity = 1.5,
		skyRotation = math.rad(90),
	},
})
```

```luau
-- An image: equirectangular (twice as wide as high, the horizon across its middle), PNG, JPEG or HDR.
local meadow = engine.assets.texture("skies/meadow.hdr")
-- ... in a Startup system:
ctx.commands:spawn({ ["aylo.Environment3D"] = { skyTexture = meadow, ambient = vector.create(0, 0, 0) } })
```

- A Radiance `.hdr` image keeps colours brighter than white, a sun for instance, which lights much better than a PNG.
  Free ones are on Poly Haven, as "HDRIs".
- With a sky lighting the world, set `ambient` low or to `0`: the sky gives the light around.
- `lightingFromSky = false` draws the sky without its light, for a stylised game that lights its world by hand.
- Changing `skyIntensity` costs nothing. Changing the image, the colours or `skyRotation` makes the sky's light again,
  a few milliseconds on the GPU: do not turn the sky every frame.
- Until the image is loaded, there is no sky.
- A game without a sky pays nothing for it.

## Shadows

A light with `castShadows = true` casts the shadows of the models in front of it onto the others, on lit materials
and on models without a material. The sun's shadows follow the camera; a lamp's, a torch's and a window's stay with
the light. They are soft where the caster is far from the surface, as real shadows are, by `shadowSoftness`: the sun's
half a degree, a bulb of 10 cm, an area light's rectangle.

```luau
ctx.commands:spawn({
	["aylo.Transform3D"] = { position = vector.create(0, 4, 0) },
	["aylo.Light3D"] = { kind = "spot", power = 800, castShadows = true, shadowSoftness = 2 },
})
ctx.commands:spawn({ ["aylo.Environment3D"] = { shadowCascades = 4, shadowResolution = 2048, shadowDistance = 80 } })
```

| What | Shadow maps |
|---|---|
| directional | one per cascade over `shadowDistance`, sharper near the camera |
| spot | one, the cone |
| area | one, a wide cone along its facing |
| point | six, all around |

The shadow maps share one image of 4096 by 4096 texels: 4 maps at a `shadowResolution` of 2048, 16 at 1024, 64 at
512. A light whose maps do not fit casts none (the lights come in the order the frame keeps them, the directional
ones first). A model chooses with `castShadows` and `receiveShadows` on its [`aylo.MeshRenderer`](3d.md), and a
shader with `#pragma shadow off` casts none (a glow, a hologram). A model's `vertex()` runs in its shadow too, so a
waving flag casts a waving shadow, and a cutout (`ALPHA_SCISSOR`, a glTF mask) casts the shape of its cut. A blended
model casts none.

## In the editor

The **Scene** tab draws each light in its colour: an arrow for the sun, three circles of a point light's range, a
spot light's cone, an area light's rectangle and the way it faces. A disabled light is grey.

## Limits

- A renderer that does not light the 3D world ignores its lights, and the output says once `[graphics] This renderer
  does not light the 3D world: its lights are ignored`.
- The lights of a glTF file (`KHR_lights_punctual`) are not read: models are drawn as one mesh, without the nodes a light
  would sit on. Place your lights with entities.
- Lights live on the client: the server spawns them as any component, and does nothing with them.
- Shadows need a 3D camera: the sun's cascades are fitted to it. The Scene tab's preview reads the game camera's.
- A sky is an equirectangular image; six cube faces are not read. A sky's light is computed at a fixed size: a very
  detailed image is drawn sharp behind the world, but lights it as a smaller one would.

## See also

- [2D and 3D](3d.md): transforms, cameras and models.
- [Lit materials](lighting.md): shaders that the lights shade.
- [Materials](materials.md): shaders of your own for models.
