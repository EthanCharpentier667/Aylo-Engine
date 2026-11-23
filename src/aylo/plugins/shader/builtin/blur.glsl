/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** blur
*/

// A Gaussian blur along one direction: run it twice, across then down, for a round blur (engine.effects.ssao and the
// anisotropic Kuwahara use it). Its reach is three times `sigma`.
#pragma screen

uniform vec2 direction = vec2(1.0, 0.0);  // (1, 0) across, (0, 1) down.
uniform float sigma = 2.0;                // In pixels of the input.

void fragment() {
    vec2 step = direction / vec2(textureSize(TEXTURE, 0));
    int reach = int(ceil(3.0 * sigma));
    vec4 sum = vec4(0.0);
    float total = 0.0;
    for (int i = -reach; i <= reach; ++i) {
        float weight = exp(-float(i * i) / (2.0 * sigma * sigma));
        sum += texture(TEXTURE, SCREEN_UV + (step * float(i))) * weight;
        total += weight;
    }
    COLOR = sum / total;
}
