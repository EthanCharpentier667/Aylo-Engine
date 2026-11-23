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
    vec4 boundsMin;     // xyz: the smallest corner of the model's box, in its space.
    vec4 boundsMax;     // xyz: the largest corner.
    vec4 instance[4];   // The draw's `instance uniform` values.
};
layout(set = 3, binding = 0, std430) readonly buffer AyloMeshDraws {
    AyloMeshDraw ayloMeshDraws[];
};
layout(push_constant) uniform AyloMeshPart {
    vec4 baseColor;  // The part's base colour, straight alpha.
    uint draw;       // The draw's index in ayloMeshDraws.
} ayloPart;

vec3 VERTEX;               // Vertex stage: the vertex, in the model's space (its entity's transform places it).
vec3 NORMAL;               // The surface's normal: in the model's space in the vertex stage, in the world's in fragment.
mat4 MODEL_MATRIX;         // The model's space to the world's.
mat4 INV_MODEL_MATRIX;     // The world's space to the model's.
mat3 NORMAL_MATRIX;        // A normal of the model's space to the world's (normalize the result).
vec3 AABB_MIN;             // The smallest corner of the model's box, in its space.
vec3 AABB_MAX;             // The largest corner.
vec3 WORLD_POSITION;       // The vertex (vertex stage, before vertex() moves it) or the pixel (fragment), in the world.
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
