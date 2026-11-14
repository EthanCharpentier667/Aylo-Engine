/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** staticmesh
*/

// Fragment stage of the static meshes, unlit: the colour (premultiplied) times the base colour texture (premultiplied,
// as the renderer makes every texture it creates).
#version 450
#extension GL_GOOGLE_include_directive : require

#include "TextureSet.glsl"

layout(location = 0) in vec2 inUv;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = inColor * texture(uTexture, inUv);
}
