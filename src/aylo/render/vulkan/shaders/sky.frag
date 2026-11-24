/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** sky
*/

// The sky, drawn first in the 3D world's pass over the whole view: the environment seen along each pixel's ray, times
// the sky's intensity. The models drawn after cover it.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "Environment.glsl"

layout(set = 0, binding = 0) uniform sampler2D environment;

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

// SkyConstants in EnvironmentMaps.cpp (the sky has a layout of its own).
layout(push_constant) uniform AyloSky {
    mat4 inverseViewProjection;  // Clip space to the world.
    vec4 values;                 // x: intensity.
} ayloSky;

void main() {
    const vec2 ndc = (inUv * 2.0) - 1.0;
    const vec4 near = ayloSky.inverseViewProjection * vec4(ndc, 0.0, 1.0);
    const vec4 far = ayloSky.inverseViewProjection * vec4(ndc, 1.0, 1.0);
    const vec3 direction = normalize((far.xyz / far.w) - (near.xyz / near.w));
    outColor = vec4(textureLod(environment, ayloEquirectUv(direction), 0.0).rgb * ayloSky.values.x, 1.0);
}
