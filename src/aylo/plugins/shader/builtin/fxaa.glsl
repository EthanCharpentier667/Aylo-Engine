/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** fxaa
*/

// Fast approximate antialiasing (after Lottes' FXAA): finds the edges by their brightness and blurs along them, which
// smooths the stairs of every edge for the cost of one pass. Put it after the tonemap: it expects colours from 0 to 1.
#pragma screen

uniform float spanMax = 8.0;      // How far along an edge it may blur, in pixels.
uniform float reduceMul = 0.125;  // How much the edge's own contrast shortens the blur.

float lumaOf(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

void fragment() {
    vec2 texel = SCREEN_PIXEL_SIZE;
    float northWest = lumaOf(texture(TEXTURE, SCREEN_UV + (vec2(-1.0, -1.0) * texel)).rgb);
    float northEast = lumaOf(texture(TEXTURE, SCREEN_UV + (vec2(1.0, -1.0) * texel)).rgb);
    float southWest = lumaOf(texture(TEXTURE, SCREEN_UV + (vec2(-1.0, 1.0) * texel)).rgb);
    float southEast = lumaOf(texture(TEXTURE, SCREEN_UV + (vec2(1.0, 1.0) * texel)).rgb);
    float middle = lumaOf(COLOR.rgb);
    float lowest = min(middle, min(min(northWest, northEast), min(southWest, southEast)));
    float highest = max(middle, max(max(northWest, northEast), max(southWest, southEast)));

    vec2 direction = vec2(-((northWest + northEast) - (southWest + southEast)),
                          (northWest + southWest) - (northEast + southEast));
    float reduce = max((northWest + northEast + southWest + southEast) * 0.25 * reduceMul, 1.0 / 128.0);
    float scale = 1.0 / (min(abs(direction.x), abs(direction.y)) + reduce);
    direction = clamp(direction * scale, vec2(-spanMax), vec2(spanMax)) * texel;

    vec3 near = 0.5 * (texture(TEXTURE, SCREEN_UV + (direction * (1.0 / 3.0 - 0.5))).rgb +
                       texture(TEXTURE, SCREEN_UV + (direction * (2.0 / 3.0 - 0.5))).rgb);
    vec3 far = (near * 0.5) + (0.25 * (texture(TEXTURE, SCREEN_UV + (direction * -0.5)).rgb +
                                       texture(TEXTURE, SCREEN_UV + (direction * 0.5)).rgb));
    float farLuma = lumaOf(far);
    COLOR.rgb = (farLuma < lowest || farLuma > highest) ? near : far;
}
