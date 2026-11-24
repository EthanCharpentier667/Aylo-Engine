/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** staticmesh_shadow
*/

// The fragment stage of the shadow maps: depth only, nothing to write. The vertex stage is the model's (staticmesh.vert,
// or a game shader's, so displaced vertices cast displaced shadows).
#version 450
#extension GL_GOOGLE_include_directive : require

#define AYLO_MESH
#include "common.glsl"

void main() {
}
