/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** bloom_up
*/

// Upsamples the image to twice its size with a 3x3 tent filter. The pipeline adds the result to what the larger image
// already holds (additive blending, alpha 1), so the glow of every level ends up in the first one.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "PostConstants.glsl"

layout(set = 0, binding = 0) uniform sampler2D source;

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

vec3 tap(vec2 offset) { return texture(source, inUv + offset * post.texelSize).rgb; }

void main() {
    vec3 color = tap(vec2(0.0, 0.0)) * 4.0;
    color += (tap(vec2(-1.0, 0.0)) + tap(vec2(1.0, 0.0)) + tap(vec2(0.0, -1.0)) + tap(vec2(0.0, 1.0))) * 2.0;
    color += tap(vec2(-1.0, -1.0)) + tap(vec2(1.0, -1.0)) + tap(vec2(-1.0, 1.0)) + tap(vec2(1.0, 1.0));
    outColor = vec4(color / 16.0, 1.0);
}
