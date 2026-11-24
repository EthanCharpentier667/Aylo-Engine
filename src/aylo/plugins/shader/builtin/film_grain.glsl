/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** film_grain
*/

// Film grain and sensor noise: a fresh random grain each frame, in brightness (film) and in colour (a digital sensor in
// the dark). Stronger in the shadows, where a real grain shows.
#pragma screen
#include "@aylo/noise.glsl"
#include "@aylo/color.glsl"

uniform float luminanceNoise = 0.06;  // How strong the grain in brightness is.
uniform float chromaNoise = 0.02;     // How strong the grain in colour is.
uniform float size = 1.5;             // A grain's size, in pixels.
uniform float speed = 24.0;           // New grains per second.

void fragment() {
    vec2 grain = floor(SCREEN_UV / (SCREEN_PIXEL_SIZE * max(size, 1.0)));
    float frame = floor(TIME * speed);
    float shade = 1.0 - clamp(luminance(COLOR.rgb), 0.0, 1.0);
    float bright = (whiteNoise(vec3(grain, frame)) - 0.5) * 2.0 * luminanceNoise;
    vec3 tint = vec3(whiteNoise(vec4(grain, frame, 1.0)), whiteNoise(vec4(grain, frame, 2.0)),
                     whiteNoise(vec4(grain, frame, 3.0))) - vec3(0.5);
    COLOR.rgb = max(COLOR.rgb + ((vec3(bright) + (tint * 2.0 * chromaNoise)) * (0.5 + shade)), vec3(0.0));
}
