/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** shape.frag
*/

// The fragment stage of shapes: the colour from the shape's signed distance (negative inside), the fill inside the
// outline band, the outline in the band [-thickness, 0], both antialiased over a pixel and premultiplied. The shape's
// kinds: 0 rounded box, 1 circle, 2 capsule along Y. With no game code, it is the default shader of shapes.
layout(location = 7) in vec2 ayloLocalIn;
layout(location = 8) flat in vec4 ayloBoundsIn;  // Half size, radius, outline thickness.
layout(location = 9) flat in uint ayloKindIn;
layout(location = 10) flat in vec4 ayloOutlineColorIn;  // Premultiplied.

// A box of half size b, its corners rounded by r.
float ayloRoundedBox(vec2 p, vec2 b, float r) {
    const vec2 q = abs(p) - (b - r);
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

// A segment along Y from -h to h, r thick on each side.
float ayloCapsule(vec2 p, float h, float r) {
    p.y -= clamp(p.y, -h, h);
    return length(p) - r;
}

float ayloSignedDistance() {
    if (ayloKindIn == 1u) {
        return length(ayloLocalIn) - ayloBoundsIn.z;
    }
    if (ayloKindIn == 2u) {
        return ayloCapsule(ayloLocalIn, max(ayloBoundsIn.y - ayloBoundsIn.z, 0.0), ayloBoundsIn.z);
    }
    return ayloRoundedBox(ayloLocalIn, ayloBoundsIn.xy, ayloBoundsIn.z);
}

#pragma aylo_shader

void main() {
    UV = ayloUvIn;
    const float ayloDistance = ayloSignedDistance();
    const float ayloPixel = max(fwidth(ayloDistance), 1e-5);
    // How much of the pixel is inside the shape, and inside its inner edge (where the fill starts).
    const float ayloShape = clamp(0.5 - (ayloDistance / ayloPixel), 0.0, 1.0);
    const float ayloFill = ayloBoundsIn.w > 0.0
                                ? clamp(0.5 - ((ayloDistance + ayloBoundsIn.w) / ayloPixel), 0.0, 1.0)
                                : ayloShape;
    // No discard outside the shape: a transparent pixel blends to nothing.
    COLOR = (ayloColorIn * ayloFill) + (ayloOutlineColorIn * (ayloShape - ayloFill));
#pragma aylo_main
    ayloFragColor = ayloFinishBlend(COLOR, ayloBlendIn & ayloBlendMask);
}
