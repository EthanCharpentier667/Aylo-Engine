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
layout(location = 3) in vec4 ayloTangent;      // In the model's space; w: the bitangent's sign.
layout(location = 4) in vec4 ayloVertexColor;  // White without one.
layout(location = 5) in vec2 ayloUv2;
layout(location = 0) out vec2 ayloUvOut;
layout(location = 1) out vec4 ayloColorOut;
layout(location = 2) flat out uint ayloBlendOut;
layout(location = 3) out vec3 ayloWorldOut;   // The vertex in the world, after vertex().
layout(location = 4) out vec3 ayloLocalOut;   // The vertex in the model's space, after vertex().
layout(location = 5) out vec4 ayloTangentOut;  // In the world; w: the bitangent's sign.
layout(location = 6) out vec4 ayloVertexColorOut;
layout(location = 8) out vec2 ayloUv2Out;
layout(location = 7) out vec3 ayloNormalOut;  // In the world.

#pragma aylo_shader

void main() {
    ayloLoadMeshDraw();
    VERTEX = ayloPosition;
    NORMAL = ayloNormal;
    // The part's texture transform (KHR_texture_transform): scale, rotate, then offset.
    const float ayloCos = cos(ayloPart.uvRotation);
    const float ayloSin = sin(ayloPart.uvRotation);
    UV = (mat2(ayloCos, -ayloSin, ayloSin, ayloCos) * (ayloUv * ayloPart.uvTransform.zw)) + ayloPart.uvTransform.xy;
    UV2 = ayloUv2;
    TANGENT = ayloTangent.xyz;
    BINORMAL = cross(ayloNormal, ayloTangent.xyz) * ayloTangent.w;
    VERTEX_COLOR = ayloVertexColor;
    COLOR = ayloMeshDraws[ayloPart.draw].color * ayloPart.baseColor * ayloVertexColor;
    WORLD_POSITION = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
#pragma aylo_main
    const vec4 ayloWorld = MODEL_MATRIX * vec4(VERTEX, 1.0);
    gl_Position = ayloFrame.viewProjection * ayloWorld;
    ayloWorldOut = ayloWorld.xyz;
    ayloLocalOut = VERTEX;
    ayloUvOut = UV;
    ayloColorOut = vec4(COLOR.rgb * COLOR.a, COLOR.a);
    ayloBlendOut = 0u;
    ayloNormalOut = NORMAL_MATRIX * NORMAL;
    ayloTangentOut = vec4(mat3(MODEL_MATRIX) * TANGENT, ayloTangent.w);
    ayloVertexColorOut = VERTEX_COLOR;
    ayloUv2Out = UV2;
}
