# Lights

Lights shine on the 3D world: the sun, a lamp, a torch, a window. You add them as components of 3D entities, and the
engine hands them to the renderer every frame, placed and turned with their entity. An unlit model (one drawn by the
built-in shader, or by a `#pragma mesh` shader of your own) ignores them; lit materials, which shade their surface from
the lights, come in the next version of the engine. A game without lights draws as before.

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
| `castShadows` | bool | `false` | whether it casts shadows (in a later version) |
| `enabled` | bool | `true` | `false` turns it off without removing it |

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

## In the editor

The **Scene** tab draws each light in its colour: an arrow for the sun, three circles of a point light's range, a
spot light's cone, an area light's rectangle and the way it faces. A disabled light is grey.

## Limits

- A renderer that does not light the 3D world ignores its lights, and the output says once `[graphics] This renderer
  does not light the 3D world: its lights are ignored`.
- The lights of a glTF file (`KHR_lights_punctual`) are not read: models are drawn as one mesh, without the nodes a light
  would sit on. Place your lights with entities.
- Lights live on the client: the server spawns them as any component, and does nothing with them.

## See also

- [2D and 3D](3d.md): transforms, cameras and models.
- [Materials](materials.md): shaders of your own for models.
