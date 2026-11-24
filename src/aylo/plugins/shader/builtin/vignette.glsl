/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** vignette
*/

// Vignette: the corners of the picture fade toward a colour, which draws the eye to the centre.
#pragma screen

uniform float intensity = 0.4;            // 0: none; 1: the corners are fully `color`.
uniform float radius = 0.75;              // Where the fade starts, from the centre: 0.5 reaches the sides' middle.
uniform float softness = 0.45;            // How wide the fade is.
uniform vec3 color = vec3(0.0, 0.0, 0.0);  // What the corners fade to.

void fragment() {
    vec2 size = 1.0 / SCREEN_PIXEL_SIZE;
    vec2 fromCenter = (SCREEN_UV - vec2(0.5)) * vec2(size.x / size.y, 1.0);
    float kept = smoothstep(radius, radius - softness, length(fromCenter));
    COLOR.rgb = mix(color, COLOR.rgb, mix(1.0, kept, intensity));
}
