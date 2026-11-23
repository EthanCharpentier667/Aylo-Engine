/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** bloom_prefilter
*/

// The first pass of the engine's mip-chain bloom (engine.effects.bloom): the part of the picture brighter than
// `threshold`, at half its size. Four bilinear taps cover four by four pixels, each weighted down by its brightness
// (Karis) so that one very bright pixel does not flicker.
#pragma screen

uniform float threshold = 1.0;  // Brightness from which a pixel glows.
uniform float softKnee = 0.5;   // 0: a hard threshold; up to 1: the glow fades in below it.

vec3 brightPart(vec3 color) {
    float brightness = max(color.r, max(color.g, color.b));
    float knee = max(threshold * softKnee, 1e-5);
    float soft = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    soft = (soft * soft) / (4.0 * knee);
    return color * (max(soft, brightness - threshold) / max(brightness, 1e-5));
}

void fragment() {
    vec2 texel = 1.0 / vec2(textureSize(TEXTURE, 0));
    vec3 sum = vec3(0.0);
    float weights = 0.0;
    for (int i = 0; i < 4; ++i) {
        vec2 offset = vec2((i & 1) == 0 ? -1.0 : 1.0, (i & 2) == 0 ? -1.0 : 1.0) * texel;
        vec3 bright = brightPart(texture(TEXTURE, SCREEN_UV + offset).rgb);
        float weight = 1.0 / (1.0 + max(bright.r, max(bright.g, bright.b)));
        sum += bright * weight;
        weights += weight;
    }
    COLOR = vec4(sum / weights, 1.0);
}
