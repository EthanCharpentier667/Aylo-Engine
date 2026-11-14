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

A model's `instance uniform` values are the ones the shader declares: a model has no `aylo.MaterialInstance` yet. The
Inspector edits a 3D material as a 2D one. A shader cannot say both `#pragma screen` and `#pragma mesh`.

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
