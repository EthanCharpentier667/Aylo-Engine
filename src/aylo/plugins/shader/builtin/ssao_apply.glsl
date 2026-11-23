/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ssao_apply
*/

// The last pass of screen-space ambient occlusion: the picture darkened where the occlusion (`occlusion`, blurred) says
// little sky is seen.
#pragma screen

uniform sampler2D occlusion;  // The blurred occlusion (engine.effects.ssao binds it).
uniform vec3 shadow = vec3(0.0, 0.0, 0.0);  // The colour the hidden corners take.

void fragment() {
    COLOR.rgb = mix(shadow, COLOR.rgb, texture(occlusion, SCREEN_UV).r);
}
