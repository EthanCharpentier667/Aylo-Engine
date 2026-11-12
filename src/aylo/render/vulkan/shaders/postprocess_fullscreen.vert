/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** postprocess_fullscreen
*/

// One triangle that covers the whole target, built from the vertex index: no vertex buffer. Shared by every
// post-process pass; the uv runs 0..1 over the target, (0, 0) at its top-left corner.
#version 450

layout(location = 0) out vec2 outUv;

void main() {
    outUv = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
    gl_Position = vec4(outUv * 2.0 - 1.0, 0.0, 1.0);
}
