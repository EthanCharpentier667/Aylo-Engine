/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** environment_source
*/

// The environment's first level: the sky as an equirectangular image, read from the game's image (turned by the sky's
// rotation) or made from the gradient's three colours.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "Environment.glsl"
#include "EnvironmentPass.glsl"

layout(set = 0, binding = 0) uniform sampler2D source;

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

vec3 skyAt(vec2 uv) {
    const vec3 seen = ayloEquirectDirection(uv);
    // What is seen at `seen` was at `seen` turned back by the rotation.
    const float c = cos(ayloPass.horizon.w);
    const float s = sin(ayloPass.horizon.w);
    const vec3 direction = vec3((c * seen.x) + (s * seen.z), seen.y, (-s * seen.x) + (c * seen.z));
    if (ayloPass.top.w > 0.5) {
        return textureLod(source, ayloEquirectUv(direction), 0.0).rgb;
    }
    const float y = direction.y;
    return y >= 0.0 ? mix(ayloPass.horizon.rgb, ayloPass.top.rgb, sqrt(y))
                    : mix(ayloPass.horizon.rgb, ayloPass.bottom.rgb, sqrt(-y));
}

void main() {
    // Four samples a texel, so a large image is averaged rather than skipped.
    const vec2 quarter = 0.25 / ayloPass.source.xy;
    const vec3 color = skyAt(inUv + vec2(-quarter.x, -quarter.y)) + skyAt(inUv + vec2(quarter.x, -quarter.y)) +
                       skyAt(inUv + vec2(-quarter.x, quarter.y)) + skyAt(inUv + vec2(quarter.x, quarter.y));
    outColor = vec4(color * 0.25, 1.0);
}
