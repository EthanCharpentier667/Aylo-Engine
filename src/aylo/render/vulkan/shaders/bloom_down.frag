/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** bloom_down
*/

// Downsamples the image to half its size with 13 taps that overlap, which keeps a bright pixel from flickering as it
// moves. The first one of the chain also keeps only what is brighter than the threshold.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "PostConstants.glsl"

layout(set = 0, binding = 0) uniform sampler2D source;

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

vec3 tap(vec2 offset) { return texture(source, inUv + offset * post.texelSize).rgb; }

// Keeps the part of the colour over the threshold, easing in over the knee.
vec3 prefilter(vec3 color) {
    // A huge value would turn the whole glow white and, as an infinity, into NaN once filtered.
    color = min(color, vec3(64.0));
    float brightness = max(color.r, max(color.g, color.b));
    float soft = clamp(brightness - post.threshold + post.knee, 0.0, 2.0 * post.knee);
    soft = soft * soft / (4.0 * post.knee + 1e-4);
    float contribution = max(soft, brightness - post.threshold) / max(brightness, 1e-4);
    return color * contribution;
}

void main() {
    vec3 a = tap(vec2(-2.0, -2.0));
    vec3 b = tap(vec2(0.0, -2.0));
    vec3 c = tap(vec2(2.0, -2.0));
    vec3 d = tap(vec2(-2.0, 0.0));
    vec3 e = tap(vec2(0.0, 0.0));
    vec3 f = tap(vec2(2.0, 0.0));
    vec3 g = tap(vec2(-2.0, 2.0));
    vec3 h = tap(vec2(0.0, 2.0));
    vec3 i = tap(vec2(2.0, 2.0));
    vec3 j = tap(vec2(-1.0, -1.0));
    vec3 k = tap(vec2(1.0, -1.0));
    vec3 l = tap(vec2(-1.0, 1.0));
    vec3 m = tap(vec2(1.0, 1.0));
    if ((post.flags & 1u) != 0u) {
        a = prefilter(a);
        b = prefilter(b);
        c = prefilter(c);
        d = prefilter(d);
        e = prefilter(e);
        f = prefilter(f);
        g = prefilter(g);
        h = prefilter(h);
        i = prefilter(i);
        j = prefilter(j);
        k = prefilter(k);
        l = prefilter(l);
        m = prefilter(m);
    }
    vec3 color = e * 0.125 + (a + c + g + i) * 0.03125 + (b + d + f + h) * 0.0625 + (j + k + l + m) * 0.125;
    outColor = vec4(color, 1.0);
}
