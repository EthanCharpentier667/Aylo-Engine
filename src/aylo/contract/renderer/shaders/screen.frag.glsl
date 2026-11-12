/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** screen.frag
*/

// The fragment stage of screen effects. TEXTURE is what the previous effect wrote (the scene, for the first one), in
// floating point, so a colour may exceed 1; SCREEN_TEXTURE is another name for it. SCREEN_UV is where this pixel is on
// the screen (0..1, top-left at 0, 0) and SCREEN_PIXEL_SIZE the size of one pixel in it. COLOR starts as the pixel of
// TEXTURE at SCREEN_UV, and what the shader leaves in it is written as is: no blending, the alpha kept. With no game
// code, it copies TEXTURE.
#define SCREEN_TEXTURE TEXTURE
vec2 SCREEN_UV;          // This pixel on the screen, 0..1.
vec2 SCREEN_PIXEL_SIZE;  // One pixel, in SCREEN_UV units: 1 / the target's size.

#pragma aylo_shader

void main() {
    UV = ayloUvIn;
    SCREEN_UV = ayloUvIn;
    SCREEN_PIXEL_SIZE = 1.0 / vec2(textureSize(TEXTURE, 0));
    COLOR = texture(TEXTURE, UV);
#pragma aylo_main
    ayloFragColor = COLOR;
}
