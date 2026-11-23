/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** mesh.frag
*/

// The fragment stage of static meshes (`#pragma mesh`): the texture times the colour, both premultiplied, unlit, as the
// built-in static mesh shader draws them.
layout(location = 7) in vec3 ayloNormalIn;  // In the world.

#pragma aylo_shader

void main() {
    ayloLoadMeshDraw();
    UV = ayloUvIn;
    NORMAL = length(ayloNormalIn) > 0.0 ? normalize(ayloNormalIn) : ayloNormalIn;
    COLOR = texture(TEXTURE, UV) * ayloColorIn;
#pragma aylo_main
    ayloFragColor = COLOR;
}
