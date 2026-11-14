/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** mesh.vert
*/

// The vertex stage of static meshes (`#pragma mesh`): a vertex of the model, placed by the draw's world matrix and seen
// through the frame's 3D camera. The draw's constants are the shared push constant range (StaticMeshConstants in the
// Vulkan renderer): the world matrix, the draw's colour and the part's base colour.
layout(location = 0) in vec3 ayloPosition;  // In the model's space.
layout(location = 1) in vec3 ayloNormal;    // In the model's space.
layout(location = 2) in vec2 ayloUv;
layout(push_constant) uniform AyloMeshDraw {
    mat4 world;      // Places the model in the world.
    vec4 color;      // The draw's colour, straight alpha.
    vec4 baseColor;  // The part's base colour, straight alpha.
} ayloDraw;
layout(location = 0) out vec2 ayloUvOut;
layout(location = 1) out vec4 ayloColorOut;
layout(location = 2) flat out uint ayloBlendOut;
layout(location = 7) out vec3 ayloNormalOut;  // In the world.

#pragma aylo_shader

void main() {
    VERTEX = ayloPosition;
    NORMAL = ayloNormal;
    UV = ayloUv;
    COLOR = ayloDraw.color * ayloDraw.baseColor;
#pragma aylo_main
    gl_Position = ayloFrame.viewProjection * ayloDraw.world * vec4(VERTEX, 1.0);
    ayloUvOut = UV;
    ayloColorOut = vec4(COLOR.rgb * COLOR.a, COLOR.a);
    ayloBlendOut = 0u;
    ayloNormalOut = mat3(ayloDraw.world) * NORMAL;
}
