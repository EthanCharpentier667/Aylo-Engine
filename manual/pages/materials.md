# Materials

A material is a shader and the values you give it: its `uniform`s and its textures. Several sprites can share one
material and change together. You can declare a material in a script, or keep it in a **`.material` file** of your
assets, which the editor opens, edits and saves (as Unity does) and the game reloads while it runs.

The shader plugin reads them: list it in `project.luau`.

```luau
engine.settings.plugins = { "aylo.graphics", "aylo.Sprite", "aylo.Shader" }
```

## A material file

```json
{
  "shader": "shaders/flash.glsl",
  "values": { "tint": [1.0, 0.2, 0.2, 1.0], "amount": 0.8 },
  "textures": { "noise": "textures/noise.png" }
}
```

Save it as `assets/materials/hit.material`, then load it in a script:

```luau
local hit = engine.assets.material("materials/hit.material")

ctx.commands:spawn({
	["aylo.Transform2D"] = { position = { x = 400, y = 300 } },
	["aylo.SpriteRenderer"] = { texture = enemy, material = hit },
})
```

| Key | Type | What it is |
|---|---|---|
| `shader` | string | The shader, a path in the assets (`shaders/flash.glsl`) or one of the engine's (`@aylo/tonemap.glsl`) |
| `values` | object | One entry per `uniform` you set: a number, or an array of 2 to 4 numbers for a `vec2` to `vec4`. A whole number given to an `int` uniform is an `int` |
| `textures` | object | One image path per `uniform sampler2D` |

What you leave out keeps the default the shader declares. The id of a material file is the id of its path, so
`engine.assets.material("materials/hit.material")` gives the same id every time you call it. On a server, which draws
nothing, it gives that id without reading the file.

A material file can also be a [screen effect](post-processing.md): give it a `#pragma screen` shader and pass it to
`engine.screenEffects`.

```json
{ "shader": "@aylo/tonemap.glsl", "values": { "mode": 2, "exposure": 1.5 } }
```

```luau
engine.screenEffects({ engine.assets.material("materials/tone.material") })
```

## One value per drawing

A `uniform` belongs to the material: every drawing that uses it shares the value. An **`instance uniform`** belongs to
each drawing: two enemies with the same material can flash by different amounts, two crates can burn at their own
pace, and they still share the material (and draw together). Declare it in the shader, then give an entity its value
with the call `aylo.shader.setInstanceValue`:

```glsl
// assets/shaders/flash.glsl
instance uniform float amount = 0.0;  // the default, for a drawing you give nothing
void fragment() { COLOR.rgb = mix(COLOR.rgb, vec3(COLOR.a), amount); }
```

```luau
local flash = engine.assets.material({ name = "flash", shader = engine.assets.shader("shaders/flash.glsl") })

-- 1 when the enemy is hit (your game sets it), fading back to 0 in a quarter of a second.
engine.ecs.component("game.Flash", { fields = { amount = "f32" } })

engine.ecs.system("game.Flash", {
	phase = "Update",
	side = "client",
	query = { write = { "game.Flash" } },
	calls = { "aylo.shader.setInstanceValue" },
	run = function(ctx)
		for entity, hit in ctx.query do
			hit.amount = math.max(hit.amount - ctx.time.delta * 4, 0)
			ctx.call("aylo.shader.setInstanceValue",
				{ entity = entity, material = flash, name = "amount", x = hit.amount })
		end
	end,
})
```

| Argument | Type | What it is |
|---|---|---|
| `entity` | entity | The entity drawn: a sprite, a text, a particle emitter or a 3D model |
| `material` | material | The material it is drawn with: its shader says where the value goes |
| `name` | string | An `instance uniform` its shader declares |
| `x` | number | The value of a `float` or `int` |
| `size`, `x`, `y`, `z`, `w` | numbers | A `vec2` to `vec4`: `size` is 2, 3 or 4, and gives that many of `x`, `y`, `z`, `w` |

The value is used from the next frame, and kept until you set it again. An entity you never give a value draws with
the defaults the shader declares; the first value you give it starts from those defaults, so you only set what
differs. Give several values in the same frame and they all count. It lives on the client only (the server draws
nothing): call it from a `side = "client"` system.

!!! warning "A value the shader does not have"
    A `name` the shader does not declare (or a value of another type) is not set, and the output says
    `[Shader] aylo.shader.setInstanceValue: ...` with the reason. A `material` that is not one is a Luau error.

A shader has 64 bytes of `instance uniform`s in all: 16 `float`s, or 4 `vec4`s.

## From vertex() to fragment()

A `varying` carries a value of yours from `vertex()` to `fragment()`: written for each vertex, read for each pixel,
blended between the vertices of its triangle. It works on every kind of drawing: sprites, texts, shapes, particles,
screen effects and models.

```glsl
#pragma mesh
varying vec3 objectPosition;  // declared once, outside the functions

void vertex() { objectPosition = VERTEX; }
void fragment() { COLOR.rgb *= 0.5 + 0.5 * fract(objectPosition.y * 4.0); }  // stripes that follow the model
```

A varying is a `float`, `vec2`, `vec3`, `vec4` or `int`. An `int` is not blended: each pixel gets the value of one of
the triangle's vertices. A shader has 5 varyings at most; a sixth is an error that names its line, as is a varying of
another type, or one named like a built-in or like another value.

## 3D materials

A model (`aylo.MeshRenderer`) takes a material too, whose shader says `#pragma mesh` on one of its first lines: a 3D
shader, compiled for models only. It is unlit, as the built-in one, and can do its own shading from the normals.

```glsl
// assets/shaders/glow.glsl
#pragma mesh
uniform vec4 tint = vec4(1.0);
uniform float swell = 0.0;
instance uniform float glow = 0.5;

void vertex() { VERTEX += NORMAL * swell; }
void fragment() { COLOR *= tint; COLOR.rgb += glow * max(NORMAL.y, 0.0); }
```

```json
{ "shader": "shaders/glow.glsl", "values": { "tint": [0.6, 1.0, 0.7, 1.0], "swell": 0.05 } }
```

```luau
ctx.commands:spawn({
	["aylo.Transform3D"] = { position = vector.create(0, 0.5, 0) },
	["aylo.MeshRenderer"] = { mesh = crate, material = engine.assets.material("materials/glow.material") },
})
```

What a 3D shader reads and writes:

| Name | Type | In `vertex()` | In `fragment()` |
|---|---|---|---|
| `VERTEX` | `vec3` | the vertex, in the model's space (the entity's transform places it after) | - |
| `NORMAL` | `vec3` | the vertex's normal, in the model's space | the surface's normal, in the world, unit |
| `UV` | `vec2` | where the texture will be read | where the texture is read |
| `COLOR` | `vec4` | the draw's colour times the part's base colour, straight alpha | the pixel's colour, **premultiplied** (texture times colour) |
| `TEXTURE` | `sampler2D` | the part's base colour texture (white without one) | the same |
| `TIME` | `float` | seconds since the renderer started | the same |
| `MODEL_MATRIX` | `mat4` | the model's space to the world's: the entity's transform | the same |
| `INV_MODEL_MATRIX` | `mat4` | the world's space to the model's | the same |
| `NORMAL_MATRIX` | `mat3` | a normal of the model's space to the world's, even under an uneven scale (normalize the result) | the same |
| `AABB_MIN` | `vec3` | the smallest corner of the box the model fits in, in its own space | the same |
| `AABB_MAX` | `vec3` | the largest corner | the same |
| `WORLD_POSITION` | `vec3` | the vertex in the world, before your `vertex()` moves it | the pixel, in the world |
| `LOCAL_POSITION` | `vec3` | - | the pixel, in the model's space (after your `vertex()`) |
| `VIEW_DIR` | `vec3` | - | unit, from the pixel toward the camera (the same everywhere for an orthographic camera) |
| `CAMERA_POSITION` | `vec3` | the 3D camera, in the world | the same |
| `VIEW_MATRIX`, `INV_VIEW_MATRIX` | `mat4` | the world's space to the camera's, and back | the same |
| `PROJECTION_MATRIX`, `INV_PROJECTION_MATRIX` | `mat4` | the camera's space to the screen's (clip space), and back | the same |
| `FRONT_FACING` | `bool` | - | whether the camera sees the triangle's front (its vertices turn counter-clockwise) |
| `FRAGCOORD` | `vec4` | - | the pixel on the screen: `xy` in pixels from the top-left corner, `z` its depth (0 near, 1 far) |

With the matrices and the box, an effect can follow the object rather than the world: a texture laid over the
model's box, a dissolve that keeps its place as the model moves. Each model gets its own `instance uniform` values,
[as any drawing](#one-value-per-drawing):

```glsl
// assets/shaders/burn.glsl
#pragma mesh
uniform sampler2D noise;
instance uniform float dissolve = 0.0;

void fragment() {
    vec3 box = (LOCAL_POSITION - AABB_MIN) / max(AABB_MAX - AABB_MIN, vec3(0.0001));  // 0 to 1 over the model
    if (texture(noise, box.xy).r < dissolve) { discard; }
}
```

With the camera, a shader shades by where the surface is seen from. A rim light brightens the edges a model shows
the camera:

```glsl
// assets/shaders/rim.glsl
#pragma mesh
uniform vec3 rimColor = vec3(0.3, 0.6, 1.0);

void fragment() {
    float rim = 1.0 - max(dot(NORMAL, VIEW_DIR), 0.0);  // 0 facing the camera, 1 on the silhouette
    COLOR.rgb += pow(rim, 3.0) * rimColor * COLOR.a * (FRONT_FACING ? 1.0 : 0.0);
}
```

The Inspector edits a 3D material as a 2D one. A shader cannot say both `#pragma screen` and `#pragma mesh`.

!!! warning "A 2D material on a model"
    A model given a material whose shader is a 2D one (no `#pragma mesh`) is drawn with the built-in shader, and the
    output says once `[Mesh] The material of a MeshRenderer has a 2D shader (...)`. A 3D shader is not drawn on
    sprites either. The raylib renderer draws no game shader: there, a model keeps the built-in look.

## Changing it while the game runs

With hot reload on, saving a `.material` file your game loaded changes every drawing that uses it in the next frame.
A file that cannot be read keeps the last version of its material, and the reload says why in the output.

## In the editor

Click a `.material` file in the **Assets** panel: the **Inspector** shows it.

- **Shader**: drop a `.glsl` file from the Assets on it to change the shader.
- **Values**: every `uniform` the shader declares, with a widget of its type, starting from the shader's default. A
  `vec3` or `vec4` whose name holds `color`, `colour` or `tint` gets a colour picker.
- **Textures**: every `uniform sampler2D`; drop an image on it.
- The `instance uniform` values are listed but not edited here: they belong to each entity, in its
  `aylo.MaterialInstance` component.

What you change shows at once on everything that uses the material. **Save** (or Ctrl+S while a material is shown and
changed) writes the file. Right-click a folder of the Assets, **New material**, to make one; give it a shader, then
values. Shaders (`.glsl`), materials (`.material`) and 3D models (`.glb`, `.gltf`, `.fbx`, `.obj`) have icons of their own in the
Assets.

## Errors

| Message | Cause |
|---|---|
| `Material 'materials/hit.material': not a JSON object` | the file is not JSON, or not an object |
| `Material '...': unknown key 'colour' (shader, values, textures)` | a key the format does not have |
| `Material '...': value 'tint' must be a number or 2 to 4 numbers` | a value of the wrong kind |
| `Material '...': texture 'noise' must be an image path` | a texture that is not a string |
| `Material '...': ...` with the reason of the file system | the file cannot be read (missing, misspelled) |
| `Shader '...': it says both #pragma screen and #pragma mesh; it can be one or the other` | both pragmas in one shader |

A value the shader does not declare, or of another type than the declared one, is reported when the material is
created, and the drawings keep the default material.

## See also

- [Post-processing](post-processing.md): writing a shader, and screen effects.
- [Setup forms](setup-forms.md): `engine.assets.shader` and `engine.assets.material`.
- [Editor](editor.md): the Assets panel and the Inspector.
