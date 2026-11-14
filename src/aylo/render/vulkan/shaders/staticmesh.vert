/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** staticmesh
*/

// Vertex stage of the static meshes (IRenderer::drawStaticMesh()): the mesh placed by its world matrix, seen through
// the frame's 3D view (set 0 holds it while the 3D pass draws).
#version 450
#extension GL_GOOGLE_include_directive : require

#include "FrameUniforms.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv;

layout(push_constant) uniform StaticMeshConstants {
    mat4 world;      // Places the mesh in the world.
    vec4 color;      // The draw's colour, linear, not premultiplied.
    vec4 baseColor;  // The part's base colour, linear, not premultiplied.
} draw;

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec4 outColor;

void main() {
    gl_Position = frame.viewProjection * draw.world * vec4(inPosition, 1.0);
    outUv = inUv;
    const vec4 color = draw.color * draw.baseColor;
    outColor = vec4(color.rgb * color.a, color.a);
}
