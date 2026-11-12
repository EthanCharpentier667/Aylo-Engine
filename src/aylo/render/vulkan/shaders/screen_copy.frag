/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** screen_copy
*/

// Copies the frame, once the screen effects ran, to where it is shown: the window, or the scene's texture of a tool.
#version 450

layout(set = 0, binding = 0) uniform sampler2D source;

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(source, inUv);
}
