/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** lines3d
*/

// Fragment stage of the 3D lines: the vertex colour, premultiplied.
#version 450

layout(location = 0) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = inColor;
}
