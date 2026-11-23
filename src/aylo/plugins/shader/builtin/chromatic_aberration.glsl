/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** chromatic_aberration
*/

// Chromatic aberration: red, green and blue land a little apart, as through a cheap lens, more toward the edges.
#pragma screen

uniform float strength = 0.004;          // The radial split: red scaled up and blue down by it, around the centre.
uniform vec2 offset = vec2(0.0, 0.0);    // A split in pixels, the same everywhere: red one way, blue the other.

void fragment() {
    vec2 fromCenter = SCREEN_UV - vec2(0.5);
    vec2 shift = offset * SCREEN_PIXEL_SIZE;
    float red = texture(TEXTURE, vec2(0.5) + (fromCenter * (1.0 + strength)) + shift).r;
    float blue = texture(TEXTURE, vec2(0.5) + (fromCenter * (1.0 - strength)) - shift).b;
    COLOR.rgb = vec3(red, COLOR.g, blue);
}
