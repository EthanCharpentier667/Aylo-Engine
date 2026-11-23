/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EnvironmentPass
*/

// The values of one pass making the environment (rendering/EnvironmentMaps.hpp).
#ifndef AYLO_ENVIRONMENT_PASS_GLSL
#define AYLO_ENVIRONMENT_PASS_GLSL

// EnvironmentPassConstants in EnvironmentMaps.cpp.
layout(push_constant) uniform AyloEnvironmentPass {
    vec4 top;      // rgb: the gradient's top; w: 1 to read the source image, 0 for the gradient.
    vec4 horizon;  // rgb: the gradient's horizon; w: the sky's rotation around Y, in radians.
    vec4 bottom;   // rgb: the gradient's bottom; w: the roughness to prefilter for, or the level to integrate.
    vec4 source;   // xy: the size of the image read (the source pass: of the image written); z: its levels.
} ayloPass;

#endif
