/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** fog
*/

// Depth fog: the 3D world fades into a colour with its distance from the camera, and thins out with height. The 2D
// world and the sky, where no 3D model was drawn, are left as they are.
#pragma screen

uniform vec3 color = vec3(0.6, 0.65, 0.7);  // The fog's colour.
uniform float density = 0.03;               // How thick it is: half the colour is fog at 0.69 / density units.
uniform float start = 0.0;                  // The distance where it starts, in world units.
uniform float height = 0.0;                 // The height below which it is thickest.
uniform float heightFalloff = 0.0;          // How fast it thins above `height`: 0, the same at every height.

void fragment() {
    float depth = texture(DEPTH_TEXTURE, SCREEN_UV).r;
    if (depth >= 1.0) {
        return;
    }
    float distance = max(linearDepth(depth) - start, 0.0);
    float amount = 1.0 - exp(-density * distance);
    if (heightFalloff > 0.0) {
        float above = max(worldPositionAt(SCREEN_UV).y - height, 0.0);
        amount *= exp(-heightFalloff * above);
    }
    COLOR.rgb = mix(COLOR.rgb, color, amount);
}
