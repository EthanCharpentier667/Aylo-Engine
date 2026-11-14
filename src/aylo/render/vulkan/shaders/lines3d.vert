/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** lines3d
*/

// Vertex stage of the 3D lines (IRenderer::drawLines3D()): points already in the world, seen through the frame's 3D
// view (set 0 holds it while the 3D pass draws). The colour comes linear and not premultiplied, and leaves
// premultiplied, as the blending of every pass expects.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "FrameUniforms.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    gl_Position = frame.viewProjection * vec4(inPosition, 1.0);
    outColor = vec4(inColor.rgb * inColor.a, inColor.a);
}
