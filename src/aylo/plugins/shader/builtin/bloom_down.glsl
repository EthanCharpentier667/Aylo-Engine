/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** bloom_down
*/

// A down step of the mip-chain bloom: the input at half its size, through 13 taps (Jimenez 2014) that keep the glow
// round and stable as it shrinks.
#pragma screen

vec3 tap(sampler2D source, vec2 uv, vec2 texel, float x, float y) {
    return texture(source, uv + (vec2(x, y) * texel)).rgb;
}

void fragment() {
    vec2 texel = 1.0 / vec2(textureSize(TEXTURE, 0));
    vec3 inner = tap(TEXTURE, SCREEN_UV, texel, -1.0, -1.0) + tap(TEXTURE, SCREEN_UV, texel, 1.0, -1.0) + tap(TEXTURE, SCREEN_UV, texel, -1.0, 1.0) + tap(TEXTURE, SCREEN_UV, texel, 1.0, 1.0);
    vec3 center = tap(TEXTURE, SCREEN_UV, texel, 0.0, 0.0);
    vec3 top = tap(TEXTURE, SCREEN_UV, texel, 0.0, -2.0);
    vec3 bottom = tap(TEXTURE, SCREEN_UV, texel, 0.0, 2.0);
    vec3 left = tap(TEXTURE, SCREEN_UV, texel, -2.0, 0.0);
    vec3 right = tap(TEXTURE, SCREEN_UV, texel, 2.0, 0.0);
    vec3 corners = tap(TEXTURE, SCREEN_UV, texel, -2.0, -2.0) + tap(TEXTURE, SCREEN_UV, texel, 2.0, -2.0) + tap(TEXTURE, SCREEN_UV, texel, -2.0, 2.0) + tap(TEXTURE, SCREEN_UV, texel, 2.0, 2.0);
    // The inner box weighs a half; four overlapping outer boxes an eighth each.
    vec3 result = (inner * 0.125) + (center * 0.125) + ((top + bottom + left + right) * 0.0625) + (corners * 0.03125);
    COLOR = vec4(result, 1.0);
}
