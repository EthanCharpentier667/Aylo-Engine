# Shaders

A game shader is a small GLSL file: a `fragment()` function, a `vertex()` function, or both, written in a dialect the
engine defines. The game never writes a pipeline or a binding. The shader plugin (`src/aylo/plugins/shader/`,
`aylo::plugins::shader`) compiles the file and hands the result to the renderer. The game adds it, after `GraphicsPlugin`, which it requires.

## Writing a shader

```glsl
// assets/shaders/flash.glsl
uniform vec4 tint = vec4(1.0, 1.0, 1.0, 1.0);  // In the material: shared by the drawings that use it.
instance uniform float amount = 0.0;           // In each drawing: every enemy has its own.
uniform sampler2D noise;                       // An extra texture, given by the material.

float pulse() { return 0.5 + (0.5 * sin(TIME * 8.0)); }  // A helper: both stages can call it.

void vertex() { VERTEX.y += pulse() * 4.0; }

void fragment() {
    COLOR.rgb = mix(COLOR.rgb, tint.rgb * COLOR.a, amount * texture(noise, UV).r);
}
```

Both functions are optional, but a shader needs at least one. What a function does not change keeps the default
behaviour of the drawing, so a shader only writes what differs. Every stage keeps only its own function, so code one
stage alone allows (`discard`, derivatives) never reaches the other.

### Built-ins

| Name      | Type        | In `vertex()`                       | In `fragment()`                                |
|-----------|-------------|-------------------------------------|------------------------------------------------|
| `VERTEX`  | `vec2`      | The vertex, in world units          | -                                              |
| `UV`      | `vec2`      | Where the texture will be read      | Where the texture is read                      |
| `COLOR`   | `vec4`      | The drawing's colour, straight alpha | The pixel's colour, **premultiplied**          |
| `TEXTURE` | `sampler2D` | The drawing's texture               | The drawing's texture                          |
| `TIME`    | `float`     | Seconds since the renderer started  | The same                                       |

`COLOR` is premultiplied in `fragment()`: multiply a new colour by the alpha, as `tint.rgb * COLOR.a` does above. The
blend mode of the drawing is applied after the function.

### Declarations

One declaration per line, outside any function:

- `uniform <type> <name> = <default>;`: a value of the **material**.
- `instance uniform <type> <name> = <default>;`: a value of **each drawing**.
- `uniform sampler2D <name>;`: an extra texture of the material, readable in both stages.
- `varying <type> <name>;`: a value `vertex()` writes and `fragment()` reads, interpolated (`float`, `vec2` to `vec4`;
  an `int` is `flat`). At most 5 (`kShaderMaxVaryings`). The assembler declares each `out` in the vertex stage and `in`
  in the fragment stage, from the first location the target leaves free (`firstVaryingLocation()`: 7 for sprites,
  glyphs, particles and the screen, 9 for meshes, 11 for shapes), so the last one is at most 15: the 16 locations
  every device offers.

The types are `float`, `vec2`, `vec3`, `vec4` and `int`. A default is a number or a `vecN(...)` of numbers (one number
fills every component), and 0 when omitted. A shader cannot declare a built-in, nor a name starting with `aylo`, which
the engine keeps for itself.

### Targets

A shader is compiled for every target that draws something (`kDrawingTargets`), so it must compile for all of them: a
failure in one fails the shader. A shader whose first lines hold `#pragma screen` is a **screen effect** instead, and is
compiled for `kScreen` only (see *Screen effects* below). One whose first lines hold `#pragma mesh` is a **3D shader**,
compiled for `kMesh` only: the parts of a static mesh (`MeshRenderer::material`), with `VERTEX` and `NORMAL` as `vec3`
(`AYLO_MESH` in `common.glsl`; the templates are `mesh.vert.glsl` and `mesh.frag.glsl`). Each draw of a mesh is an
`AyloMeshDraw` in a storage buffer of the frame (set 3, binding 0, std430: the model matrix, its inverse, the normal
matrix, the draw's colour, the model's bounds and the draw's instance values), indexed by the push constants
`AyloMeshPart` (the part's base colour and the draw's index). Both stages fill `MODEL_MATRIX`, `INV_MODEL_MATRIX`,
`NORMAL_MATRIX`, `AABB_MIN` and `AABB_MAX` from it (`ayloLoadMeshDraw()`), and read the instance values from it.
The 3D camera of the frame is in `AyloFrame` (`view`, `projection`, their inverses, `cameraPosition`, whose `w` is 1 for
a perspective and 0 for an orthographic projection; identity and the 2D view-projection in 2D frames): a mesh reads it as
`VIEW_MATRIX`, `PROJECTION_MATRIX`, `INV_VIEW_MATRIX`, `INV_PROJECTION_MATRIX` and `CAMERA_POSITION`. The mesh
templates pass the world and model-space positions at locations 3 and 4: `WORLD_POSITION` (in the vertex stage, the
vertex before `vertex()`), `LOCAL_POSITION`, and `VIEW_DIR` (toward the camera, or along the orthographic camera's +Z).
A 3D shader also says how it is drawn, with pragmas of its first lines, read by the parser into
`CompiledShader::renderState` (`contract::renderer::RenderState`):

| Pragma | Values | Default |
|---|---|---|
| `#pragma cull` | `back`, `front`, `none` | `back` |
| `#pragma blend` | `opaque`, `alpha`, `premultiplied` (the same factors as `alpha`: `COLOR` is premultiplied), `additive`, `multiply` | `opaque` |
| `#pragma depth` | `test write`, `test nowrite`, `notest` | `test write`, or `test nowrite` for a blended shader |
| `#pragma depth_bias` | a number of depth steps, and a slope factor | `0 0` |

An unknown value is an error naming its line, and so is one of these pragmas in a shader that is not `#pragma mesh`. A
material replaces parts of it (`Material::renderState`, a `RenderStateOverride`: from a `.material` file's
`renderState`, the call `aylo.shader.setMaterialRenderState`, or the Inspector); `ShaderHandles` merges it with the
shader's into `MaterialDesc::renderState`. `ALPHA_SCISSOR`, written in `fragment()`, makes a cutout: a pixel whose
alpha is below it is discarded, the others are opaque. The assembler compiles the test in (`AYLO_ALPHA_SCISSOR`) only
when the shader names it, since a `discard` can cost the early depth test. The built-in mesh shader keeps its own state
(no culling, premultiplied blending, depth written) until R3D-07 reads glTF's `doubleSided`.

A material's next pass (`Material::nextPass`, a `.material` file's `nextPass`, the call
`aylo.shader.setMaterialNextPass`) is drawn after it on the same parts of a model. `loadMaterialFile()` reads the next
passes of a file as files of their own and refuses a chain that loops or holds more than `kMaxMaterialPasses` (8); the
mesh plugin follows the chain each frame, one `drawStaticMesh()` per pass with `listedPartsOnly`, and cuts a chain of
materials made in code at 8 with one warning. Passes only apply to models.

`#pragma lit` makes a 3D shader lit: its `fragment()` writes the surface (`ALBEDO`, `ALPHA`, `METALLIC`, `ROUGHNESS`,
`SPECULAR`, `NORMAL`, `EMISSION`, `AO`) and the engine lights it after (`lighting.glsl`, added to the fragment stage
with `AYLO_LIT`); an optional `void light()` replaces the shading of each light (`AYLO_CUSTOM_LIGHT`), and is blanked
from the vertex stage as `fragment()` is. The model and its units are in `docs/3d/rendering.md` (Lit materials).

`FRONT_FACING` is `gl_FrontFacing`: the projection puts Y down in clip space and the screen too, so a triangle
counter-clockwise in the world is counter-clockwise on screen, the pipelines' default front face. `FRAGCOORD`
(`gl_FragCoord`) is in every fragment stage. The
built-in mesh shader (`staticmesh.*` in the Vulkan backend) is these templates with no game code. Both pragmas in one
shader is an error.

| Target      | Drawn on                    | `UV`                                    | `COLOR` in `fragment()` before the shader  |
|-------------|-----------------------------|-----------------------------------------|--------------------------------------------|
| `kSprite`   | Sprites and tiles           | The texture coordinates                 | The texture times the colour               |
| `kGlyph`    | The glyphs of a `Text`      | The atlas coordinates                   | The colour, covering the glyph (MSDF)      |
| `kParticle` | Particles, on CPU or GPU    | The texture coordinates, by flipbook    | The texture times the colour               |
| `kShape`    | `Shape`s                    | The position in the shape's box, 0 to 1 | The antialiased fill and outline           |
| `kScreen`   | The whole screen, once      | The position on the screen, 0 to 1      | The pixel of the picture so far            |
| `kMesh`     | The parts of a static mesh  | The vertex's texture coordinates        | The part's texture times its colours       |

`UV` goes a little past 0 and 1 on a shape, in the margin of its antialiased edge. A shape has no texture: `TEXTURE`
reads a white pixel there.

Sprites, shapes, texts and particles are all drawn with game shaders. A `TextRenderer` names its material as a
`SpriteRenderer` does. A `ParticleEffect` names one for all its emitters, and a `ParticleEmitter` can replace it. The
`instance uniform` values of a text or an emitter come from its entity's `MaterialInstance`, and are shared by all
its glyphs or particles.

### Screen effects

A screen effect (`#pragma screen`) runs over the whole picture once the world is drawn: a fullscreen triangle into a
half-float image, reading the scene or the previous effect's output. Its built-ins:

| Name                | Type        | What it is                                                                     |
|---------------------|-------------|--------------------------------------------------------------------------------|
| `COLOR`             | `vec4`      | Starts as the pixel of `SCREEN_TEXTURE` at `SCREEN_UV`; written as is, no blend |
| `SCREEN_TEXTURE`    | `sampler2D` | The picture so far (half float, may exceed 1); `TEXTURE` is the same           |
| `SCREEN_UV`         | `vec2`      | The pixel on the screen, 0 to 1, top-left at (0, 0); `UV` is the same          |
| `SCREEN_PIXEL_SIZE` | `vec2`      | One pixel in `SCREEN_UV` units                                                 |
| `TIME`              | `float`     | As everywhere                                                                  |

`VERTEX` is in pixels of the screen in `vertex()`. An `instance uniform` keeps its declared default: no drawing carries
values, so the assembler writes the defaults into the vertex stage. The `SCREEN_*` names are reserved in every shader
but only declared in a screen effect.

The game chooses its effects with `aylo.shader.setScreenMaterials` (or the form `engine.screenEffects`), a list of
materials kept in the client resource `aylo.ScreenMaterials`. The Render system `aylo.Shader.Screen`, after
`aylo.Shader.Update`, hands the renderer the ones created (`IRenderer::setScreenMaterials()`) every frame, so a reload
or a late material follows. The renderer skips a material whose shader does not serve `kScreen`.

The plugin gives two effects written in the same dialect, compiled from `shader/builtin/` (`BuiltinShaders`): a path
starting with `@aylo/` names one of them instead of a file. `@aylo/bloom.glsl` (one pass, 64 samples on a spiral, with
`threshold`, `softKnee`, `intensity`, `radius`) and `@aylo/tonemap.glsl` (`exposure`, and `mode` 0 none, 1 Reinhard,
2 ACES). The manual's page is `manual/pages/post-processing.md`.

## Using a shader

A drawing does not name a shader but a **material**: a shader and the values of its `uniform`s and textures. Several
materials can share a shader. A drawing that names no material gets the **default material**, whose shader is the
built-in one of its target.

```cpp
void spawnEnemy(ecs::ResMut<ShaderAssets> shaders, ecs::ResMut<MaterialAssets> materials,
                ecs::ResMut<TextureAssets> textures, ecs::Commands& commands) {
    const AssetId flash = shaders->load("assets/shaders/flash.glsl");
    const AssetId hit = materials->add("enemy_hit", Material{.shader = flash}
                                                        .set("tint", glm::vec4{1.0F, 0.2F, 0.2F, 1.0F})
                                                        .setTexture("noise", textures->load("assets/noise.png")));
    const CompiledShader* compiled = shaders->get(flash);
    commands.spawn()
        .add(Transform{})
        .add(SpriteRenderer{.texture = enemy, .material = hit})
        .add(MaterialInstance::of(*compiled).set(*compiled, "amount", 0.8F));
}
```

- **`ShaderAssets`** (the resource `aylo.ShaderAssets`) keeps the shaders by `AssetId`. `load()` reads and compiles a
  file, and `add()` a shader written in code. Both give an id at once. A shader that does not compile is recorded as
  failed: `get()` gives nullptr and `getEntry()` the error, with its line in the file.
- **`MaterialAssets`** keeps the materials: a shader and the values of its `uniform`s and textures, shared by every
  drawing that uses it. What a material does not set keeps the shader's defaults.
- **`SpriteRenderer::material`** names the material of a sprite.
- **`MaterialInstance`** (the component `aylo.MaterialInstance`) holds an entity's own `instance uniform` values.
  Without it, the entity gets the declared defaults. Sprites that share a material and a texture stay in one batch,
  whatever their values, so one enemy flashes without a material of its own.
- **`ShaderHandles`** creates the shaders and materials in the renderer, in the Render phase, before the frame begins.
  A material waits for its shader and its textures: until then, the drawing keeps the default material.

A `Shape` has no component: it is drawn by calling `IRenderer::draw()`, so it takes its material from `ShaderHandles`.

```cpp
if (const ShaderHandles::Material* material = handles->getMaterial(hit)) {
    backend->renderer->draw(Shape{.kind = ShapeKind::kCircle, .position = {100.0F, 100.0F}, .radius = 16.0F,
                                  .material = material->material, .instanceValues = material->instanceDefaults});
}
```

`examples/plugins/shader` shows sprites and shapes with both kinds of values.

### From a script

`engine.assets.shader` and `engine.assets.material` do the same from a game's scripts. A value is a number or a vector
`{x, y}`, `{x, y, z}` or `{x, y, z, w}`; a whole number fills an `int`. The form gives the material's id, for the
`material` field of a `SpriteRenderer`. On a server, which has no ShaderPlugin, both forms only give the id.

```lua
local flash = engine.assets.shader("shaders/flash.glsl")
local hit = engine.assets.material({
    name = "enemy_hit",
    shader = flash,
    values = { amount = 0.8, tint = { x = 1, y = 0.2, z = 0.2, w = 1 } },
    textures = { noise = engine.assets.texture("noise.png") },
})
```

### Material files

A material can be a `.material` file of the assets (`MaterialFile.hpp`, plan in `docs/materials/plan.md`): JSON with
`shader` (a path), `values` (a number or 2 to 4 numbers by name) and `textures` (an image path by name).
`loadMaterialFile(path, shaders, materials, textures)` reads it through the shaders' `FileSource`, loads its shader and
textures, gives a whole number an `int` when the shader declares one, and makes it the material known by its path
(`MaterialAssets::define()`, which replaces it when read again). `describeMaterial()` gives it back as a `MaterialFile`
and `MaterialFile::write()` as JSON, which is how the editor saves one. Scripts load it with
`engine.assets.material("materials/hit.material")` (the call `aylo.shader.loadMaterial`). nlohmann::json stays private
to the plugin. `MaterialReloadable` reads again, with hot reload on, the `.material` files the game loaded, anywhere in
its assets; a file that cannot be read keeps the last version.

### Where the files are

In a game, a shader's path starts in the game's assets, as every asset's does (`engine/design.md`, *Assets*):
`shaders/flash.glsl` is `<game>/assets/shaders/flash.glsl`. In a program without an `Engine`, as most examples, it is
relative to the working directory.

## Hot reload

With hot reload on, saving a shader changes the running game. The `ShaderPlugin` adds a `ShaderReloadable` to the
engine's `HotReloader` (the `aylo.HotReload` resource, `engine/design.md`, *Reloading*), which looks at
`<game>/assets/shaders` and its subfolders by the files' modification times and logs each reload:
`[HotReload] shaders: applied in 4.1 ms (modified assets/shaders/flash.glsl): shader shaders/flash.glsl`.

```lua
-- project.luau: true by default in a debug build, false in a release one; one switch for the whole engine.
engine.settings.hotReload = true
```

- Only the shaders the game loaded are compiled. Another file in the folder is ignored until the game asks for it, and a
  shader loaded from elsewhere, or written in code, is not followed.
- A shader keeps its `AssetId`, so its materials follow it: `ShaderHandles` creates the new shader, then its materials,
  in the same frame. A material whose values no longer match the shader is reported and drawn with the default material
  until the next save.
- An edit that does not compile prints the error and keeps the last version on screen. A shader that failed at load
  is created at the first save that compiles.
- A `MaterialInstance` keeps its bytes: if a save moves or retypes an `instance uniform`, set its values again.

## Limits

| What                              | Limit                                                    |
|-----------------------------------|----------------------------------------------------------|
| `instance uniform` of a shader    | 64 bytes in all (`kShaderInstanceBytes`): 16 `float`, or 4 `vec4` |
| `uniform sampler2D` of a shader   | 3 (`kShaderMaxTextures`), besides the drawing's texture  |
| `uniform` of a material           | Not bound by these 64 bytes: they are shared, in a block of the material |

The limits only concern the data of a drawing, not what a shader does. 64 bytes hold what usually differs between two
drawings: an amount, a tint, a progress, a seed. When a shader needs more:

- What several drawings share goes in a `uniform` of the material.
- A large amount of data goes in a texture, read with `texture()`.
- A single object with its own settings can have its own material, at the price of a batch of its own.

The size is the same for every target, and it can only grow a little: the values of sprites and shapes travel as vertex
attributes, of which a device only has to offer 16, and those of particles in a block limited to 128 bytes on some
devices, 48 of which are taken.

## The contract with the backends

The shader plugin is one producer of shaders among others. A renderer knows nothing of it, and only sees two things,
both in `contract/renderer/`:

- **`CompiledShader`** (`Shader.hpp`): plain data. The SPIR-V of each target, and what reflection found: the values, the
  textures, their offsets and defaults. `IRenderer::createShader()` takes it, and `createMaterial()` a shader, the bytes
  of its block and its textures.
- **The shaders ABI** (`shaders/*.glsl`): the GLSL a shader is assembled in. A stage is made of `common.glsl`, the
  target's other files and the shader's code, placed where the `#pragma aylo_shader` and `#pragma aylo_main` markers
  are. Those markers are ignored by GLSL.

The **default shader** of a target is the ABI with no game code: the same files, compiled as they are. A backend gets
its built-in shaders by writing `#version 450` and including them, so built-in and game drawings run the same code, and
it never depends on the shader plugin, which can be replaced.

A backend feeds these resources:

| What                                | Where                                                             |
|-------------------------------------|-------------------------------------------------------------------|
| The frame's values                  | Set 0, binding 0: time, delta time, framebuffer size, view-projection |
| The drawing's texture               | Set 1, binding 0                                                  |
| The material's values               | Set 2, binding 0 (`std140`)                                       |
| The material's textures             | Set 2, bindings 1 to 3                                            |
| Sprite and glyph instances          | Attributes 0 to 6, instance values at 7 to 10                     |
| Shape instances                     | Attributes 0 to 7, instance values at 8 to 11                     |
| Particle instances                  | Attributes 0 to 5                                                 |
| A particle draw's constants         | A push constant block, instance values at offset 48               |
| A screen effect                     | No vertex input: 3 vertices; set 1 binding 0 is the picture so far |
| A static mesh's vertices            | Attributes 0 to 2: position, normal, UV (`StaticMeshVertex`)      |
| The frame's lights (meshes)          | Set 3, binding 1 (std430, `AyloLights`: ambient, count, `AyloLight`s) |
| The shadow maps (meshes)            | Set 3, binding 2 (std430, `AyloShadowTile`s), the atlas at binding 3 |
| A static mesh's draws               | Set 3, binding 0 (std430, `AyloMeshDraw`), indexed by the push constants `AyloMeshPart` (base colour, draw index) |

Instance values only exist for a shader that declares some: the assembler then defines `AYLO_INSTANCE_VALUES`.
A script gives an entity its own with the call `aylo.shader.setInstanceValue` (`entity`, `material`, `name`, a value):
it queues the value in the client resource `aylo.InstanceValueRequests`, and the Render system
`aylo.Shader.InstanceValues` (before `aylo.Shader.Update`) writes the frame's values into the entities'
`MaterialInstance`, adding one from the shader's defaults to an entity without it, so several values given to a new
entity in one frame all count (`InstanceValues.hpp`).

SPIR-V is the format every backend starts from. A backend for another API translates it, for example with SPIRV-Cross,
mapping each set and binding to its own resources.

## How a shader is compiled

1. **The parser** (`detail/ShaderParser`) reads the declarations and lays the values out. It leaves their lines empty,
   so an error keeps the line of the file.
2. **The assembler** (`detail/ShaderAssembler`) builds the complete GLSL 450 of each stage of each target. Each stage
   keeps only its own function of the shader.
3. **The compiler** (`detail/ShaderCompiler`) turns it into SPIR-V with glslang, at load time.

A renderer builds a shader's pipeline the first time it is drawn.
