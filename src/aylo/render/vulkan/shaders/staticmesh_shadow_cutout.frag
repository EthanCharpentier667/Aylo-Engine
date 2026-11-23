/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** staticmesh_shadow_cutout
*/

// The fragment stage of the shadow maps for a part cut out by its alpha (glTF's MASK): a leaf casts the shadow of a
// leaf. The alpha is the base colour's, as the built-in material cuts it.
#version 450
#extension GL_GOOGLE_include_directive : require

#define AYLO_MESH
#include "common.glsl"

layout(location = 0) in vec2 ayloUvIn;
layout(location = 1) in vec4 ayloColorIn;  // Premultiplied.

void main() {
    if (texture(TEXTURE, ayloUvIn).a * ayloColorIn.a < ayloPart.surface.w) {
        discard;
    }
}
