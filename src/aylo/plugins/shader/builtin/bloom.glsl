/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** bloom
*/

// The engine's bloom, a screen effect: what is brighter than `threshold` glows around itself. Load it as
// "@aylo/bloom.glsl", or copy it into your game to change it. One pass: each pixel gathers the bright part of the
// pixels in a disc of `radius` pixels around it (a spiral of samples, weighted to fade out at the edge).
#pragma screen

uniform float threshold = 1.0;   // Brightness from which a pixel glows: above 1 only glows what is lit past white.
uniform float softKnee = 0.5;    // 0: a hard threshold; up to 1: the glow fades in below it.
uniform float intensity = 0.35;  // How much of the glow is added to the scene.
uniform float radius = 24.0;     // How far the glow reaches, in pixels.

// The part of `color` that glows: the threshold with a soft knee below it.
vec3 brightPart(vec3 color) {
    float brightness = max(color.r, max(color.g, color.b));
    float knee = max(threshold * softKnee, 1e-5);
    float soft = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    soft = (soft * soft) / (4.0 * knee);
    return color * (max(soft, brightness - threshold) / max(brightness, 1e-5));
}

void fragment() {
    const int taps = 64;
    const float goldenAngle = 2.39996323;
    vec3 glow = vec3(0.0);
    float total = 0.0;
    for (int i = 0; i < taps; ++i) {
        float distance = sqrt((float(i) + 0.5) / float(taps));
        float angle = float(i) * goldenAngle;
        vec2 offset = vec2(cos(angle), sin(angle)) * distance * radius * SCREEN_PIXEL_SIZE;
        float weight = exp(-4.0 * distance * distance);
        glow += brightPart(texture(SCREEN_TEXTURE, SCREEN_UV + offset).rgb) * weight;
        total += weight;
    }
    COLOR.rgb += (glow / total) * intensity;
}
