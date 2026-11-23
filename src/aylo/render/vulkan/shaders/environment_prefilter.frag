/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** environment_prefilter
*/

// A level of the sky's reflection, prefiltered for one roughness (Karis 2013, split sum): the environment seen through
// GGX lobes around each direction, the view along it. Samples from blurrier levels of the environment for wider lobes,
// so that few samples do not show as noise.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "Environment.glsl"
#include "EnvironmentPass.glsl"

layout(set = 0, binding = 0) uniform sampler2D environment;

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const uint kSamples = 128u;

float radicalInverse(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

void main() {
    const vec3 n = ayloEquirectDirection(inUv);
    const float roughness = ayloPass.bottom.w;
    if (roughness < 0.01) {
        outColor = vec4(textureLod(environment, inUv, 0.0).rgb, 1.0);
        return;
    }
    const float alpha = roughness * roughness;
    const vec3 up = abs(n.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    const vec3 tangent = normalize(cross(up, n));
    const vec3 bitangent = cross(n, tangent);
    const float texelAngle = 4.0 * AYLO_ENVIRONMENT_PI / (ayloPass.source.x * ayloPass.source.y);
    vec3 sum = vec3(0.0);
    float weights = 0.0;
    for (uint i = 0u; i < kSamples; ++i) {
        const vec2 xi = vec2(float(i) / float(kSamples), radicalInverse(i));
        const float phi = 2.0 * AYLO_ENVIRONMENT_PI * xi.x;
        const float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (((alpha * alpha) - 1.0) * xi.y)));
        const float sinTheta = sqrt(1.0 - (cosTheta * cosTheta));
        const vec3 h = (tangent * (sinTheta * cos(phi))) + (bitangent * (sinTheta * sin(phi))) + (n * cosTheta);
        const vec3 l = (2.0 * dot(n, h) * h) - n;
        const float nol = dot(n, l);
        if (nol <= 0.0) {
            continue;
        }
        // The sample's share of the sphere, against a texel's: the level whose texels are that big.
        const float noh = cosTheta;
        const float d = (alpha * alpha) / (AYLO_ENVIRONMENT_PI * pow((noh * noh * ((alpha * alpha) - 1.0)) + 1.0, 2.0));
        const float pdf = d / 4.0;
        const float sampleAngle = 1.0 / ((float(kSamples) * pdf) + 1e-4);
        const float level = clamp((0.5 * log2(sampleAngle / texelAngle)) + 1.0, 0.0, ayloPass.source.z - 1.0);
        sum += textureLod(environment, ayloEquirectUv(l), level).rgb * nol;
        weights += nol;
    }
    outColor = vec4(sum / max(weights, 1e-4), 1.0);
}
