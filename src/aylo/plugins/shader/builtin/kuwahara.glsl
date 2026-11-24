/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** kuwahara
*/

// The classic Kuwahara filter: each pixel takes the mean colour of the least varied of the four squares around it, which
// flattens textures into patches of paint and keeps the edges. engine.effects.kuwahara({ anisotropic = true }) runs the
// anisotropic one instead.
#pragma screen

uniform int radius = 4;  // The squares' side, in pixels: larger, broader strokes.

void fragment() {
    vec3 means[4];
    float variances[4];
    for (int quadrant = 0; quadrant < 4; ++quadrant) {
        ivec2 side = ivec2((quadrant & 1) == 0 ? -1 : 1, (quadrant & 2) == 0 ? -1 : 1);
        vec3 sum = vec3(0.0);
        vec3 squares = vec3(0.0);
        for (int y = 0; y <= radius; ++y) {
            for (int x = 0; x <= radius; ++x) {
                vec3 color = texture(TEXTURE, SCREEN_UV + (vec2(ivec2(x, y) * side) * SCREEN_PIXEL_SIZE)).rgb;
                sum += color;
                squares += color * color;
            }
        }
        float count = float((radius + 1) * (radius + 1));
        means[quadrant] = sum / count;
        vec3 variance = abs((squares / count) - (means[quadrant] * means[quadrant]));
        variances[quadrant] = variance.r + variance.g + variance.b;
    }
    int best = 0;
    for (int quadrant = 1; quadrant < 4; ++quadrant) {
        if (variances[quadrant] < variances[best]) {
            best = quadrant;
        }
    }
    COLOR.rgb = means[best];
}
