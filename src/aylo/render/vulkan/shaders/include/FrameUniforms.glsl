/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FrameUniforms
*/

// What every shader can read during a frame: the GPU side of rendering/FrameUniforms.hpp, which must match it field
// by field (std140 layout). Change both files together.
//
// Use it from a shader with:
//     #extension GL_GOOGLE_include_directive : require
//     #include "FrameUniforms.glsl"
// then read frame.time, frame.deltaTime, frame.framebufferSize and frame.viewProjection.

#ifndef AYLO_FRAME_UNIFORMS_GLSL
#define AYLO_FRAME_UNIFORMS_GLSL

layout(set = 0, binding = 0, std140) uniform FrameUniforms {
    float time;            // Seconds since the renderer's init(). Offset 0.
    float deltaTime;       // Seconds since the previous frame, 0 on the first one. Offset 4.
    vec2 framebufferSize;  // Size of the rendered images, in pixels. Offset 8.
    mat4 viewProjection;   // World to clip space: the frame's camera. Offset 16.
    mat4 view;             // World to camera space (identity in 2D). Offset 80.
    mat4 projection;       // Camera to clip space. Offset 144.
    mat4 inverseView;      // Camera to world space. Offset 208.
    mat4 inverseProjection;  // Clip to camera space. Offset 272.
    vec4 cameraPosition;   // xyz: the camera in the world; w: 1 perspective, 0 orthographic. Offset 336.
} frame;

#endif
