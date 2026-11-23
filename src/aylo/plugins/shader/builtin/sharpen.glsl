/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** sharpen
*/

// Sharpen: each pixel pushed away from the mean of its neighbours, which brings out edges and fine detail.
#pragma screen

uniform float amount = 0.5;  // 0: unchanged; 1 and more: strong. Too much rings around edges.
uniform int mode = 0;        // 0: box, the eight neighbours; 1: diamond, the four sharing a side (softer).

vec3 tap(sampler2D source, vec2 uv, vec2 texel, float x, float y) {
    return texture(source, uv + (vec2(x, y) * texel)).rgb;
}

void fragment() {
    vec3 sides = tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, -1.0, 0.0) + tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, 1.0, 0.0) + tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, 0.0, -1.0) + tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, 0.0, 1.0);
    vec3 neighbours = sides / 4.0;
    if (mode == 0) {
        neighbours = (sides + tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, -1.0, -1.0) + tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, 1.0, -1.0) + tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, -1.0, 1.0) + tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, 1.0, 1.0)) / 8.0;
    }
    COLOR.rgb = max(COLOR.rgb + ((COLOR.rgb - neighbours) * amount), vec3(0.0));
}
