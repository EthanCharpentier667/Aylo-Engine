/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** mesh.vert
*/

// The vertex stage of static meshes (`#pragma mesh`): a vertex of the model, placed by the draw's model matrix and seen
// through the frame's 3D camera. The draw's data (common.glsl, AYLO_MESH) is in the frame's buffer of mesh draws.
layout(location = 0) in vec3 ayloPosition;  // In the model's space.
layout(location = 1) in vec3 ayloNormal;    // In the model's space.
layout(location = 2) in vec2 ayloUv;
layout(location = 0) out vec2 ayloUvOut;
layout(location = 1) out vec4 ayloColorOut;
layout(location = 2) flat out uint ayloBlendOut;
layout(location = 7) out vec3 ayloNormalOut;  // In the world.

#pragma aylo_shader

void main() {
    ayloLoadMeshDraw();
    VERTEX = ayloPosition;
    NORMAL = ayloNormal;
    UV = ayloUv;
    COLOR = ayloMeshDraws[ayloPart.draw].color * ayloPart.baseColor;
#pragma aylo_main
    gl_Position = ayloFrame.viewProjection * MODEL_MATRIX * vec4(VERTEX, 1.0);
    ayloUvOut = UV;
    ayloColorOut = vec4(COLOR.rgb * COLOR.a, COLOR.a);
    ayloBlendOut = 0u;
    ayloNormalOut = NORMAL_MATRIX * NORMAL;
}
