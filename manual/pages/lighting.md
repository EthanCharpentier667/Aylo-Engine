# Lit materials

A lit material is a 3D shader that says what its surface is (its colour, how metallic and rough it is, which way it
faces) and lets the engine light it with the [lights](lights.md) of the scene: the sun, lamps, torches, windows, and
the ambient light. Its shader says `#pragma lit` after `#pragma mesh`, and writes the surface in `fragment()`.

```glsl
// assets/shaders/painted.glsl
#pragma mesh
#pragma lit
uniform sampler2D albedoMap;
uniform vec3 tint = vec3(1.0);

void fragment() {
    ALBEDO = texture(albedoMap, UV).rgb * tint;
    ROUGHNESS = 0.4;
    METALLIC = 0.0;
}
```

```luau
local painted = engine.assets.material({
	name = "painted",
	shader = engine.assets.shader("shaders/painted.glsl"),
	textures = { albedoMap = engine.assets.texture("textures/wood.png") },
})
ctx.commands:spawn({
	["aylo.Transform3D"] = {},
	["aylo.MeshRenderer"] = { mesh = crate, material = painted },
})
```

A model with no material is lit too as soon as the scene has a light or an `aylo.Environment3D`: its colours and
textures are its albedo, rough and not metallic. Without any, it is drawn unlit, as before. A `#pragma mesh` shader
without `#pragma lit` stays unlit: its `COLOR` is what is drawn.

## The surface

`fragment()` starts with the surface of the model's part (its colours and texture) and changes what it wants:

| Name | Type | Starts as | Meaning |
|---|---|---|---|
| `ALBEDO` | `vec3` | the part's colours times its texture | the base colour, linear, not premultiplied |
| `ALPHA` | `float` | their alpha | opacity, for a blended material (`#pragma blend alpha`) |
| `METALLIC` | `float` | `0` | `0` a dielectric (wood, plastic, skin), `1` a metal |
| `ROUGHNESS` | `float` | `0.5` | `0` a mirror, `1` fully rough |
| `SPECULAR` | `float` | `0.5` | a dielectric's reflectance: `0.5` reflects 4% head-on, as glTF |
| `NORMAL` | `vec3` | the surface's normal, in the world, turned toward the camera on a back face | write it to bump the surface: a normal map, a procedural ripple |
| `EMISSION` | `vec3` | `0` | light the surface gives off, added as it is (may exceed 1: a neon) |
| `AO` | `float` | `1` | ambient occlusion: how much of the ambient light reaches it |

Every other built-in of a [3D shader](materials.md#3d-materials) still works (`WORLD_POSITION`, `VIEW_DIR`, `UV`...).
The engine lights the surface after `fragment()`, with the metallic-roughness model of glTF 2.0: GGX reflections,
height-correlated Smith visibility, Schlick Fresnel, Lambert diffusion. A material looks the same as in Blender's
Principled BSDF or any glTF viewer. Changing `NORMAL` changes the lighting:

```glsl
#pragma mesh
#pragma lit
void fragment() {
    NORMAL = normalize(NORMAL + 0.15 * sin(WORLD_POSITION * 12.0));  // hammered metal
    METALLIC = 1.0;
    ROUGHNESS = 0.25;
}
```

The picture is in linear light and may go past white: add the engine's tonemap as a
[screen effect](post-processing.md) to bring it back, as a camera does.

## Models from 3D tools

A model's own materials are used as they are: a glTF file exported from Blender (or any glTF viewer's sample) looks as
it does there, without a shader of yours. Every part keeps:

| From the file | What it does |
|---|---|
| base colour and its texture | the albedo, times the vertex colours if the file has some |
| metallic and roughness, and their texture (roughness in green, metallic in blue) | how metallic and rough it is |
| normal map, and its scale | bumps the surface |
| occlusion texture, and its strength | the ambient occlusion |
| emissive colour, texture and strength (`KHR_materials_emissive_strength`) | the light it gives off |
| alpha mode: opaque, mask (with its cutoff), blend | opaque ignores the alpha, mask cuts the part out, blend sees through it, drawn back to front |
| double sided | both faces drawn and lit; otherwise its back faces are skipped |
| texture transform (`KHR_texture_transform`, the base colour's) | moves, scales and turns every texture of the part |

An FBX or OBJ file gives its base colour, metalness, roughness, emission, normal and occlusion maps the same way
(ufbx turns every FBX shading model into these values); it is drawn from both sides, and see-through when its
opacity is below 1. Normal maps use tangents: the file's, or ones the engine makes (MikkTSpace, as Blender and the
baking tools do) when it has none. A model with no normal map carries no tangents.

A `#pragma lit` shader of yours on such a model starts from the same surface: its `fragment()` changes only what it
wants (a tint over the file's albedo, a wetter roughness).

## The vertices' data

| Name | Type | In `vertex()` | In `fragment()` |
|---|---|---|---|
| `TANGENT` | `vec3` | along the texture's U, in the model's space | in the world, unit |
| `BINORMAL` | `vec3` | along V, in the model's space | in the world, unit |
| `VERTEX_COLOR` | `vec4` | the vertex's colour (white without one), linear | blended between the vertices |
| `UV2` | `vec2` | the second set of texture coordinates (`0, 0` without one): lightmaps, decals | the same |

```glsl
#pragma mesh
#pragma lit
uniform sampler2D lightmap;
void fragment() {
    ALBEDO = COLOR.rgb * VERTEX_COLOR.rgb;
    AO = texture(lightmap, UV2).r;
}
```

`TANGENT` is along X without a normal map in the file. `COLOR` already holds the vertex colours, multiplied in.

## Your own shading: `light()`

A stylised game (toon bands, painterly, a custom fall-off) writes a `light()` function: it replaces the engine's
shading of each light, and is called once per light that reaches the pixel. It adds what that light gives to
`DIFFUSE_LIGHT` and `SPECULAR_LIGHT`; the engine adds the ambient light and the emission after.

```glsl
#pragma mesh
#pragma lit
uniform float bands = 3.0;

void fragment() { ROUGHNESS = 1.0; }

void light() {
    float lit = floor(max(dot(NORMAL, LIGHT_DIR), 0.0) * bands) / bands;
    DIFFUSE_LIGHT += ALBEDO * LIGHT_COLOR * ATTENUATION * SHADOW * lit;
}
```

| Name | Type | In `light()` |
|---|---|---|
| `LIGHT_DIR` | `vec3` | unit, from the surface toward the light (toward the centre of an area light) |
| `LIGHT_COLOR` | `vec3` | the light's colour times its strength here, facing it (W/m²): `LIGHT_COLOR * dot(NORMAL, LIGHT_DIR)` is what reaches the surface |
| `ATTENUATION` | `float` | how much of it reaches this point: the distance, the range, a spot's cone |
| `SHADOW` | `float` | `1` lit, `0` in shadow (always `1` until shadows come) |
| `DIFFUSE_LIGHT` | `vec3` | what the lights gave so far by diffusion: add to it |
| `SPECULAR_LIGHT` | `vec3` | what the lights gave so far by reflection: add to it |

The surface built-ins (`ALBEDO`, `NORMAL`, `ROUGHNESS`...) hold what `fragment()` wrote. A `light()` in a shader
without `#pragma lit` is an error, as is `#pragma lit` without `#pragma mesh`.

## How the lights reach a surface

| Light | What reaches a surface facing it |
|---|---|
| directional | its `intensity` (lux), everywhere |
| point | `power / (4π)` over the distance squared, brought to 0 at its `range` |
| spot | the same, inside its cone: full inside `innerConeAngle`, fading to nothing at `outerConeAngle` |
| area | the light of the whole rectangle (`power / (π × width × height)` from each point of it): soft, and reflected as a long highlight on shiny surfaces |

The ambient light of `aylo.Environment3D` lights everything, diffused by the albedo and reflected by shiny surfaces,
times `AO`.

## Errors

| Message | Cause |
|---|---|
| `Shader '...': '#pragma lit' is for a '#pragma mesh' shader` | `#pragma lit` in a 2D shader |
| `Shader '...': 'void light()' shades a lit surface: the shader needs '#pragma lit'` | a `light()` in an unlit shader |
| `Shader '...', line ...: 'ALBEDO' is reserved by the engine` | a value named as a built-in |
| a GLSL error naming `ALBEDO` | a surface built-in in a shader without `#pragma lit` |

## See also

- [Lights](lights.md): the lights and the ambient light.
- [Materials](materials.md): material files, values, render states, passes.
- [Post-processing](post-processing.md): the tonemap.
