/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ssao
*/

// The first pass of screen-space ambient occlusion (engine.effects.ssao): how much of the sky each 3D point sees, from
// the depth of its neighbours, in red (1: open, 0: closed in). Points in a hemisphere around the surface's normal are
// tested against the depth: those behind what was drawn are hidden.
#pragma screen
#include "@aylo/math.glsl"

uniform float radius = 0.5;     // How far around a point it looks, in world units.
uniform float intensity = 1.0;  // How dark the hidden corners get.
uniform float bias = 0.025;     // A depth margin, in world units, against the surface shading itself.
uniform int samples = 16;       // Points tested per pixel: more, smoother and slower.

void fragment() {
    float depth = texture(DEPTH_TEXTURE, SCREEN_UV).r;
    if (depth >= 1.0) {
        COLOR = vec4(1.0);
        return;
    }
    vec3 position = viewPositionAt(SCREEN_UV, depth);
    vec3 normal = normalAt(SCREEN_UV);
    uint seed = randomBits(uint(SCREEN_UV.x / SCREEN_PIXEL_SIZE.x) + (uint(SCREEN_UV.y / SCREEN_PIXEL_SIZE.y) * 4099u));
    float hidden = 0.0;
    for (int i = 0; i < samples; ++i) {
        float z = (random(seed, uint(3 * i)) * 2.0) - 1.0;
        float turn = random(seed, uint((3 * i) + 1)) * TAU;
        float ring = sqrt(max(1.0 - (z * z), 0.0));
        vec3 direction = vec3(ring * cos(turn), ring * sin(turn), z);
        direction = dot(direction, normal) < 0.0 ? -direction : direction;
        float scale = float(i + 1) / float(samples);
        vec3 probe = position + (direction * radius * mix(0.1, 1.0, scale * scale));
        vec4 clip = PROJECTION_MATRIX * vec4(probe, 1.0);
        vec2 uv = ((clip.xy / clip.w) * 0.5) + 0.5;
        if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0) {
            continue;
        }
        float surface = viewPositionAt(uv).z;
        float near = smoothstep(0.0, 1.0, radius / max(abs(position.z - surface), 1e-4));
        hidden += (surface >= probe.z + bias ? 1.0 : 0.0) * near;
    }
    float open = clamp(1.0 - ((hidden / float(max(samples, 1))) * intensity), 0.0, 1.0);
    COLOR = vec4(open, open, open, 1.0);
}
