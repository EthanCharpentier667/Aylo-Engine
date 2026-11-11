/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** sprite.frag
*/

// The fragment stage of sprites and particles: the texture times the colour, both premultiplied. With no game code, it
// is the default shader of sprites and particles.
#pragma aylo_shader

void main() {
    UV = ayloUvIn;
    COLOR = texture(TEXTURE, UV) * ayloColorIn;
#pragma aylo_main
    ayloFragColor = ayloFinishBlend(COLOR, ayloBlendIn & ayloBlendMask);
}
