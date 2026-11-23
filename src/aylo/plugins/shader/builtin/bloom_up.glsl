/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** bloom_up
*/

// An up step of the mip-chain bloom: the smaller level, blurred by a 3x3 tent as it grows back, added to this level's
// down step (`level`).
#pragma screen

uniform sampler2D level;      // This size's down step.
uniform float scatter = 1.0;  // How far the tent reaches, in pixels of the smaller level: wider is softer.

vec3 tap(sampler2D source, vec2 uv, vec2 texel, float x, float y) {
    return texture(source, uv + (vec2(x, y) * texel)).rgb;
}

void fragment() {
    vec2 texel = scatter / vec2(textureSize(TEXTURE, 0));
    vec3 tent = tap(TEXTURE, SCREEN_UV, texel, -1.0, -1.0) + tap(TEXTURE, SCREEN_UV, texel, 1.0, -1.0) + tap(TEXTURE, SCREEN_UV, texel, -1.0, 1.0) + tap(TEXTURE, SCREEN_UV, texel, 1.0, 1.0);
    tent += (tap(TEXTURE, SCREEN_UV, texel, 0.0, -1.0) + tap(TEXTURE, SCREEN_UV, texel, -1.0, 0.0) + tap(TEXTURE, SCREEN_UV, texel, 1.0, 0.0) + tap(TEXTURE, SCREEN_UV, texel, 0.0, 1.0)) * 2.0;
    tent += tap(TEXTURE, SCREEN_UV, texel, 0.0, 0.0) * 4.0;
    COLOR = vec4(texture(level, SCREEN_UV).rgb + (tent / 16.0), 1.0);
}
