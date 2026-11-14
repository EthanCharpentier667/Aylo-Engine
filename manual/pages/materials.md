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
values. Shaders (`.glsl`), materials (`.material`) and 3D models (`.glb`, `.gltf`) have icons of their own in the
Assets.

## Errors

| Message | Cause |
|---|---|
| `Material 'materials/hit.material': not a JSON object` | the file is not JSON, or not an object |
| `Material '...': unknown key 'colour' (shader, values, textures)` | a key the format does not have |
| `Material '...': value 'tint' must be a number or 2 to 4 numbers` | a value of the wrong kind |
| `Material '...': texture 'noise' must be an image path` | a texture that is not a string |
| `Material '...': ...` with the reason of the file system | the file cannot be read (missing, misspelled) |

A value the shader does not declare, or of another type than the declared one, is reported when the material is
created, and the drawings keep the default material.

## See also

- [Post-processing](post-processing.md): writing a shader, and screen effects.
- [Setup forms](setup-forms.md): `engine.assets.shader` and `engine.assets.material`.
- [Editor](editor.md): the Assets panel and the Inspector.
