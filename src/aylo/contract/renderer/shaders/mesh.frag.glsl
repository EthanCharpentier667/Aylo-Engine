/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** mesh.frag
*/

// The fragment stage of static meshes (`#pragma mesh`): the texture times the colour, both premultiplied, unlit, as the
// built-in static mesh shader draws them.
layout(location = 3) in vec3 ayloWorldIn;
layout(location = 4) in vec3 ayloLocalIn;
layout(location = 7) in vec3 ayloNormalIn;  // In the world.
vec3 LOCAL_POSITION;  // The pixel, in the model's space.
vec3 VIEW_DIR;        // Unit, from the pixel to the camera (a perspective), or toward an orthographic camera.
#define FRONT_FACING gl_FrontFacing  // Whether the triangle is seen from its front (counter-clockwise) side.
#ifdef AYLO_ALPHA_SCISSOR
float ALPHA_SCISSOR;  // A pixel whose COLOR.a is below it is not drawn, the others are opaque (cutout); 0: off.
#endif

#pragma aylo_shader

void main() {
    ayloLoadMeshDraw();
    WORLD_POSITION = ayloWorldIn;
    LOCAL_POSITION = ayloLocalIn;
    VIEW_DIR = ayloFrame.cameraPosition.w > 0.5 ? normalize(ayloFrame.cameraPosition.xyz - ayloWorldIn)
                                                : normalize(ayloFrame.inverseView[2].xyz);
    UV = ayloUvIn;
    NORMAL = length(ayloNormalIn) > 0.0 ? normalize(ayloNormalIn) : ayloNormalIn;
    COLOR = texture(TEXTURE, UV) * ayloColorIn;
#ifdef AYLO_ALPHA_SCISSOR
    ALPHA_SCISSOR = 0.0;
#endif
#pragma aylo_main
#ifdef AYLO_ALPHA_SCISSOR
    if (ALPHA_SCISSOR > 0.0) {
        if (COLOR.a < ALPHA_SCISSOR) {
            discard;
        }
        COLOR = vec4(COLOR.rgb / COLOR.a, 1.0);
    }
#endif
    ayloFragColor = COLOR;
}
