/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** fragment_common
*/

// The start of every fragment stage: its inputs and output, and the blend modes (the premultiplied-alpha blend).
layout(location = 0) in vec2 ayloUvIn;
layout(location = 1) in vec4 ayloColorIn;  // Premultiplied.
layout(location = 2) flat in uint ayloBlendIn;
layout(location = 0) out vec4 ayloFragColor;

const uint ayloBlendAdditive = 1u;
const uint ayloBlendCutout = 2u;
const uint ayloBlendMask = 0xFFu;

// What the premultiplied-alpha blend needs to draw `color` (premultiplied) with `blend`.
vec4 ayloFinishBlend(vec4 color, uint blend) {
    if (blend == ayloBlendAdditive) {
        return vec4(color.rgb, 0.0);
    }
    if (blend == ayloBlendCutout) {
        return color.a < 0.5 ? vec4(0.0) : vec4(color.rgb / color.a, 1.0);
    }
    return color;
}
