/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** tonemap
*/

// The engine's tonemap, a screen effect: brings the floating-point scene, where a colour may exceed 1, back to what a
// screen shows. Load it as "@aylo/tonemap.glsl", or copy it into your game to change it. Put it last: an effect after
// it gets colours between 0 and 1 again.
#pragma screen

uniform float exposure = 1.0;  // Multiplies the scene first: above 1 brighter, below 1 darker.
uniform int mode = 2;          // 0: none (only the exposure, clamped), 1: Reinhard, 2: ACES (filmic).

void fragment() {
    vec3 color = max(COLOR.rgb * exposure, vec3(0.0));
    if (mode == 1) {
        color = color / (vec3(1.0) + color);
    } else if (mode == 2) {
        // Krzysztof Narkowicz's fit of the ACES curve.
        color = (color * ((2.51 * color) + 0.03)) / ((color * ((2.43 * color) + 0.59)) + 0.14);
    }
    COLOR = vec4(clamp(color, 0.0, 1.0), 1.0);
}
