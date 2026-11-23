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

### How a 3D material is drawn

A 3D shader says how its pixels are drawn with pragmas on its first lines, and a material can change them:

```glsl
// assets/shaders/glass.glsl
#pragma mesh
#pragma blend alpha   // see what is behind it
#pragma cull none     // and its inside faces too
void fragment() { COLOR = vec4(0.2, 0.5, 1.0, 1.0) * 0.4; }  // premultiplied: rgb times the alpha
```

| Pragma | Values | Default |
|---|---|---|
| `#pragma cull` | `back`: the faces seen from behind are not drawn; `front`: the faces seen from the front are not drawn; `none`: both sides drawn | `back` |
| `#pragma blend` | `opaque`; `alpha` (or `premultiplied`, the same): over what is behind by `COLOR.a`; `additive`: added to it (fire, glows); `multiply`: multiplies it (stains, tinted glass) | `opaque` |
| `#pragma depth` | `test write`: hidden by what is in front, hides what is behind; `test nowrite`: hidden, hides nothing; `notest`: always drawn | `test write`, and `test nowrite` with a `blend` other than `opaque` |
| `#pragma shadow off` | (no value) | the models drawn with it cast no shadow |
| `#pragma depth_bias` | a number, and optionally a slope factor: moves the depth by that many steps (negative is closer), to draw a decal on a wall without flicker | `0 0` |

A model with a blended material is drawn after every opaque one, from the farthest to the nearest, so glass in front
of glass looks right from every side. Two blended models that cross each other can still show the wrong one in front
where they cross: split one of them, or let the denser one write depth (`#pragma depth test write`).

!!! note "Back faces"
    A 3D shader skips the faces seen from behind (`#pragma cull back`), which halves the work on closed models. A
    plane or a leaf seen from both sides needs `#pragma cull none`. A face's front is the side its vertices turn
    counter-clockwise, as in glTF, FBX and OBJ files. Models without a material are drawn with both sides for now.

**Cutout.** Write `ALPHA_SCISSOR` in `fragment()` to cut a shape out of a texture (leaves, fences, grass): a pixel
whose `COLOR.a` is below it is not drawn, the others are opaque. It stays an opaque material, sorted with them.

```glsl
#pragma mesh
#pragma cull none
void fragment() { ALPHA_SCISSOR = 0.5; }  // the texture's alpha cuts the leaf
```

**An outline** (an inverted hull): the model drawn a little bigger, its front faces skipped, so only the rim behind the
model shows. Draw it as a second entity at the same place, with the same model.

```glsl
// assets/shaders/outline.glsl
#pragma mesh
#pragma cull front
uniform vec4 outlineColor = vec4(0.0, 0.0, 0.0, 1.0);
uniform float width = 0.03;
void vertex() { VERTEX += NORMAL * width; }
void fragment() { COLOR = outlineColor; }
```

A material changes what it wants of its shader's way of drawing, in its file or in the script that makes it; what it
leaves out keeps the shader's pragma:

```json
{ "shader": "shaders/glass.glsl", "renderState": { "blend": "additive", "depthBias": -1 } }
```

```luau
local ghost = engine.assets.material({
	name = "ghost",
	shader = engine.assets.shader("shaders/glass.glsl"),
	renderState = { blend = "additive", cull = "back" },
})
```

| `renderState` key | Value |
|---|---|
| `cull` | `"back"`, `"front"` or `"none"` |
| `blend` | `"opaque"`, `"alpha"`, `"premultiplied"`, `"additive"` or `"multiply"` |
| `depth` | `"test write"`, `"test nowrite"` or `"notest"` |
| `depthBias`, `depthBiasSlope` | numbers |

A name it does not know is an error: `Material '...': render state 'cull' is "back", "front" or "none"` in a file,
`'sideways' is not a render state cull` from a script. A render state pragma in a shader without `#pragma mesh` is an
error too: 2D drawings have their own blend modes.

The Inspector edits a 3D material as a 2D one, and shows its **Render state**: each part is the shader's until you
pick another.

### Several materials on one model

A model is made of **parts**: a glTF primitive, or the faces of one FBX or OBJ material (a car's body, its windows,
its lights). `material` draws every part; `materials` gives some of them their own, in the order of the file, the first
part at `[1]`. A part you leave out, or give no material, keeps `material`:

```luau
local car = engine.assets.mesh("models/car.glb")
local paint = engine.assets.material("materials/paint.material")
local glass = engine.assets.material("materials/glass.material")
local spawned = false

engine.ecs.system("game.SpawnCar", {
	phase = "Update",
	side = "client",
	query = {},
	calls = { "aylo.mesh.findPart" },
	run = function(ctx)
		-- the part whose material the file names "Glass"; 0 until the file is read
		local windows = ctx.call("aylo.mesh.findPart", { mesh = car, name = "Glass" })
		if windows == 0 or spawned then
			return
		end
		spawned = true
		local materials = {}
		for part = 1, windows do
			materials[part] = paint
		end
		materials[windows] = glass
		ctx.commands:spawn({
			["aylo.Transform3D"] = {},
			["aylo.MeshRenderer"] = { mesh = car, material = paint, materials = materials },
		})
	end,
})
```

`aylo.mesh.findPart` gives the index of the part whose material has that name in the file (as Blender names it), from
1, or 0 when no part has that name or the file is not read yet (a model loads over its first frames). When you know the
order of the parts, write the list directly: `materials = { paint, glass }`.

### Passes

A material can name a **next pass**: another material drawn right after it, on the same parts of the same model,
wherever the model goes. It is how an outline, a highlight of the selected unit or a shield goes on every model that
uses the material, without a second entity:

```json
{ "shader": "shaders/painted.glsl", "nextPass": "materials/outline.material" }
```

```luau
local outline = engine.assets.material("materials/outline.material")
local selected = engine.assets.material({ name = "selected", shader = painted, nextPass = outline })
```

The next pass can have a next pass of its own: a part draws 8 materials at most, its own and 7 passes. A longer
chain is cut, and the output says `[Mesh] A material's next passes chain more than 8 materials` (two materials that name
each other loop forever otherwise). A `.material` file whose next passes come back to it is refused when it is read:
`Material 'materials/a.material': its next passes loop back to 'materials/a.material'`. A material cannot be its own
next pass. The passes only apply to models, and the Inspector sets one when you drop a `.material` on **Next pass**. A shader cannot say both `#pragma screen` and `#pragma mesh`.

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
| `Material '...': unknown key 'colour' (shader, values, textures, renderState)` | a key the format does not have |
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
