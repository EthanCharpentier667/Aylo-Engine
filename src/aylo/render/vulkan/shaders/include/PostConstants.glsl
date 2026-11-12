/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PostConstants
*/

// The push constants of the post-process passes: the CPU side is PostConstants (postprocess/BloomPlan.hpp), change both
// together. Each pass reads the fields it needs.
layout(push_constant) uniform PostConstants {
    vec2 texelSize;   // 1 / size of the image the pass reads, in pixels.
    float threshold;  // Brightness from which a colour glows (prefilter).
    float knee;       // Width of the soft transition under the threshold, in brightness (prefilter).
    float intensity;  // Weight of the glow added back (composite).
    float exposure;   // Multiplier before the tonemap (composite).
    int tonemap;      // 0 none, 1 Reinhard, 2 ACES (composite).
    uint flags;       // Bit 0: the first downsample, which keeps only the bright part.
}
post;
