/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** common
*/

// The shaders ABI of the renderer contract: game shaders are assembled in it (plugins/shader), and backends write their
// built-in shaders against it. The start of every stage of every shader: the frame (set 0, binding 0), the drawing's
// texture (set 1, binding 0) and the built-ins a shader reads and writes. Every name of the engine starts with aylo,
// a prefix shaders cannot use. Kept ASCII: it is embedded as chars. It has no #version: the assembler writes it first,
// as a backend's built-in shaders (the default shader, which #include these files) do.

layout(set = 0, binding = 0, std140) uniform AyloFrame {
    float time;            // Seconds since the renderer started.
    float deltaTime;       // Seconds since the previous frame.
    vec2 framebufferSize;  // In pixels.
    mat4 viewProjection;   // World to clip space: the frame's camera.
    mat4 view;             // World to camera space (identity in 2D).
    mat4 projection;       // Camera to clip space.
    mat4 inverseView;      // Camera to world space.
    mat4 inverseProjection;  // Clip to camera space.
    vec4 cameraPosition;   // xyz: the camera in the world; w: 1 for a perspective, 0 for an orthographic projection.
} ayloFrame;
layout(set = 1, binding = 0) uniform sampler2D TEXTURE;
#define TIME ayloFrame.time

#ifdef AYLO_MESH
// What one static mesh draw is (StaticMeshDrawData in the Vulkan renderer, std430): the frame's draws in a storage
// buffer at set 3, binding 0, indexed by the push constants' `draw`, so both stages read the model's matrices, bounds
// and instance values. The part's base colour travels with the index: it changes per part, the rest per draw.
struct AyloMeshDraw {
    mat4 model;         // The model's space to the world's.
    mat4 inverseModel;  // The world's space to the model's.
    mat4 normal;        // The inverse transpose of the model matrix (its upper 3x3 is used): normals to the world.
    vec4 color;         // The draw's colour, straight alpha.
    vec4 boundsMin;     // xyz: the smallest corner of the model's box, in its space; w: 1 if shadows fall on it.
    vec4 boundsMax;     // xyz: the largest corner.
    vec4 instance[4];   // The draw's `instance uniform` values.
};
layout(set = 3, binding = 0, std430) readonly buffer AyloMeshDraws {
    AyloMeshDraw ayloMeshDraws[];
};
// The frame's lights (LightData.hpp in the Vulkan renderer, std430) at set 3, binding 1: the ambient light, then
// `count.x` lights in the world's space, laid out as a clustered pass would read them.
struct AyloLight {
    vec4 position;   // xyz: where it is; w: its range, 0 for none.
    vec4 direction;  // xyz: where it shines, unit; w: its kind (0 directional, 1 point, 2 spot, 3 area).
    vec4 color;      // rgb: linear colour; w: its intensity, in watts or lux.
    vec4 cone;       // x, y: cos of the spot's inner and outer half-angles; z, w: half the area's size.
    vec4 right;      // xyz: the area's width axis, unit; w: its first shadow map + 1, 0 without.
};
// The frame's shadow maps (ShadowPlan.hpp in the Vulkan renderer, std430) at set 3, binding 2, tiles of the atlas at
// binding 3: a directional light has one per cascade, a point light six (+X, -X, +Y, -Y, +Z, -Z).
struct AyloShadowTile {
    mat4 viewProjection;  // World to the light's clip space.
    vec4 rect;            // xy: its corner in the atlas, zw: its size.
    vec4 bias;            // x: depth bias, y: normal bias (texels), z: slope bias.
    vec4 depth;           // x, y: near and far; z: 1 perspective, 0 orthographic; w: a cascade's far end.
    vec4 size;            // x: world width (orthographic) or tan of half its angle; y: the light's size; z: a texel.
};
layout(set = 3, binding = 2, std430) readonly buffer AyloShadows {
    AyloShadowTile tile[];
} ayloShadows;
layout(set = 3, binding = 3) uniform sampler2D ayloShadowAtlas;
// The sky's light (EnvironmentMaps.hpp in the Vulkan renderer), equirectangular: its reflection, prefiltered for
// roughness in `count.z` mips, at binding 4; its irradiance at binding 5. Read only while `count.z` is not 0.
layout(set = 3, binding = 4) uniform sampler2D ayloSkySpecularMap;
layout(set = 3, binding = 5) uniform sampler2D ayloSkyDiffuseMap;
layout(set = 3, binding = 1, std430) readonly buffer AyloLights {
    vec4 ambient;  // rgb: the ambient light; w: the sky's intensity as light.
    uvec4 count;   // x: how many lights follow; y: the cascades of a directional light; z: the sky's mips, 0: no sky.
    AyloLight light[];
} ayloLights;
// The part being drawn: its material (glTF's metallic-roughness), with the draw's index.
layout(push_constant) uniform AyloMeshPart {
    vec4 baseColor;    // The part's base colour, straight alpha.
    vec4 emissive;     // rgb: the light it gives off; w: the normal map's scale.
    vec4 surface;      // x: metallic, y: roughness, z: occlusion strength, w: alpha cutoff.
    vec4 uvTransform;  // xy: offset, zw: scale of its texture coordinates.
    uint draw;         // The draw's index in ayloMeshDraws.
    uint flags;        // Which maps it has (ayloPartHas*), and its alpha mode in bits 4 and 5.
    float uvRotation;  // The rotation of its texture coordinates, radians.
    uint padding;
} ayloPart;
const uint ayloPartHasMetallicRoughness = 1u;
const uint ayloPartHasNormal = 2u;
const uint ayloPartHasOcclusion = 4u;
const uint ayloPartHasEmissive = 8u;
// The part's maps besides its base colour (TEXTURE), at set 1, bindings 1 to 4.
layout(set = 1, binding = 1) uniform sampler2D ayloPartMetallicRoughness;
layout(set = 1, binding = 2) uniform sampler2D ayloPartNormal;
layout(set = 1, binding = 3) uniform sampler2D ayloPartOcclusion;
layout(set = 1, binding = 4) uniform sampler2D ayloPartEmissive;

vec3 VERTEX;               // Vertex stage: the vertex, in the model's space (its entity's transform places it).
vec3 NORMAL;               // The surface's normal: in the model's space in the vertex stage, in the world's in fragment.
mat4 MODEL_MATRIX;         // The model's space to the world's.
mat4 INV_MODEL_MATRIX;     // The world's space to the model's.
mat3 NORMAL_MATRIX;        // A normal of the model's space to the world's (normalize the result).
vec3 AABB_MIN;             // The smallest corner of the model's box, in its space.
vec3 AABB_MAX;             // The largest corner.
vec3 WORLD_POSITION;       // The vertex (vertex stage, before vertex() moves it) or the pixel (fragment), in the world.
vec3 TANGENT;              // Along U: in the model's space in the vertex stage, in the world's (unit) in the fragment.
vec3 BINORMAL;             // Along V, as TANGENT.
vec4 VERTEX_COLOR;         // The vertex's colour (white without one), linear.
vec2 UV2;                  // The second set of texture coordinates (lightmaps); 0 without one.
#define CAMERA_POSITION ayloFrame.cameraPosition.xyz  // The 3D camera, in the world.
#define VIEW_MATRIX ayloFrame.view                     // World to camera space.
#define PROJECTION_MATRIX ayloFrame.projection         // Camera to clip space.
#define INV_VIEW_MATRIX ayloFrame.inverseView          // Camera to world space.
#define INV_PROJECTION_MATRIX ayloFrame.inverseProjection  // Clip to camera space.

// Fills the built-ins of the draw, at the start of both stages' main().
void ayloLoadMeshDraw() {
    MODEL_MATRIX = ayloMeshDraws[ayloPart.draw].model;
    INV_MODEL_MATRIX = ayloMeshDraws[ayloPart.draw].inverseModel;
    NORMAL_MATRIX = mat3(ayloMeshDraws[ayloPart.draw].normal);
    AABB_MIN = ayloMeshDraws[ayloPart.draw].boundsMin.xyz;
    AABB_MAX = ayloMeshDraws[ayloPart.draw].boundsMax.xyz;
}
#else
vec2 VERTEX;  // Vertex stage: the vertex, in world units.
#endif
vec2 UV;      // Where the texture is read.
vec4 COLOR;   // Vertex stage: the drawing's colour; fragment stage: the pixel's, premultiplied.
