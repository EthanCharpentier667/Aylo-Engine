/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** postprocess_composite
*/

// Brings the HDR scene to the window: adds the glow, applies the exposure and the tonemap. The window's format
// encodes the result, as it did the colours drawn straight into it.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "PostConstants.glsl"

layout(set = 0, binding = 0) uniform sampler2D scene;
layout(set = 1, binding = 0) uniform sampler2D bloom;

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

vec3 reinhard(vec3 color) { return color / (1.0 + color); }

// Narkowicz's fit of the ACES filmic curve.
vec3 aces(vec3 color) {
    return clamp((color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 color = texture(scene, inUv).rgb;
    if (post.intensity > 0.0) {
        color += texture(bloom, inUv).rgb * post.intensity;
    }
    color *= post.exposure;
    if (post.tonemap == 1) {
        color = reinhard(color);
    } else if (post.tonemap == 2) {
        color = aces(color);
    }
    outColor = vec4(color, 1.0);
}
