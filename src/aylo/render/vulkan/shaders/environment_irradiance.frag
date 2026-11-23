/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** environment_irradiance
*/

// The sky's irradiance for each direction a surface may face, divided by pi: what a white matte surface facing it shows.
// Every texel of a small level of the environment, weighted by its solid angle and its cosine.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "Environment.glsl"
#include "EnvironmentPass.glsl"

layout(set = 0, binding = 0) uniform sampler2D environment;

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const int kWidth = 32;
const int kHeight = 16;

void main() {
    const vec3 n = ayloEquirectDirection(inUv);
    const float level = ayloPass.bottom.w;
    const float cell = (2.0 * AYLO_ENVIRONMENT_PI / float(kWidth)) * (AYLO_ENVIRONMENT_PI / float(kHeight));
    vec3 sum = vec3(0.0);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            const vec2 uv = (vec2(float(x), float(y)) + 0.5) / vec2(float(kWidth), float(kHeight));
            const vec3 l = ayloEquirectDirection(uv);
            const float cosine = max(dot(n, l), 0.0);
            const float solidAngle = cell * sin(uv.y * AYLO_ENVIRONMENT_PI);
            sum += textureLod(environment, uv, level).rgb * cosine * solidAngle;
        }
    }
    outColor = vec4(sum / AYLO_ENVIRONMENT_PI, 1.0);
}
