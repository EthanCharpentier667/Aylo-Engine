/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** kuwahara_tensor
*/

// The first pass of the anisotropic Kuwahara filter (Kyprianidis et al. 2009): the structure tensor of the picture,
// from the Sobel gradients of each channel: (gx.gx, gy.gy, gx.gy) in red, green and blue.
#pragma screen

vec3 tap(sampler2D source, vec2 uv, vec2 texel, float x, float y) {
    return texture(source, uv + (vec2(x, y) * texel)).rgb;
}

void fragment() {
    vec3 gx = ((tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, 1.0, -1.0) + (2.0 * tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, 1.0, 0.0)) + tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, 1.0, 1.0)) -
               (tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, -1.0, -1.0) + (2.0 * tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, -1.0, 0.0)) + tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, -1.0, 1.0))) / 4.0;
    vec3 gy = ((tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, -1.0, 1.0) + (2.0 * tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, 0.0, 1.0)) + tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, 1.0, 1.0)) -
               (tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, -1.0, -1.0) + (2.0 * tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, 0.0, -1.0)) + tap(TEXTURE, SCREEN_UV, SCREEN_PIXEL_SIZE, 1.0, -1.0))) / 4.0;
    COLOR = vec4(dot(gx, gx), dot(gy, gy), dot(gx, gy), 1.0);
}
