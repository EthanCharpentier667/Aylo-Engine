/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** bloom_composite
*/

// The last pass of the mip-chain bloom: the glow, the sum of its levels, added to the picture.
#pragma screen

uniform sampler2D bloom;          // The last up step.
uniform float intensity = 0.35;   // How much of the glow is added.
uniform float levels = 4.0;       // How many levels the glow sums, to bring it back to one.

void fragment() {
    COLOR.rgb += texture(bloom, SCREEN_UV).rgb * (intensity / levels);
}
