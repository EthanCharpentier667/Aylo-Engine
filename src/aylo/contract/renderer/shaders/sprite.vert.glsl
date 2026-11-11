/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** sprite.vert
*/

// The vertex stage of sprites and glyphs: a SpriteInstance per instance, a unit quad of 6 vertices built from
// gl_VertexIndex, scaled, turned around its origin and seen through the frame's camera. With no game code, it is the
// default shader of sprites and glyphs.
layout(location = 0) in vec2 ayloPosition;   // World point the origin sits on.
layout(location = 1) in vec2 ayloSize;       // In world units.
layout(location = 2) in vec2 ayloOrigin;     // Pivot from the top-left corner, in world units.
layout(location = 3) in float ayloRotation;  // Radians, clockwise on screen.
layout(location = 4) in vec4 ayloUvRect;     // (u0, v0, u1, v1).
layout(location = 5) in vec4 ayloColor;      // Multiplies the texture.
layout(location = 6) in uint ayloBlend;      // The blend mode, and the MSDF distance range of a glyph.
#ifdef AYLO_INSTANCE_VALUES
layout(location = 7) in vec4 ayloInstance[4];  // The drawing's instance values.
#endif
layout(location = 0) out vec2 ayloUvOut;
layout(location = 1) out vec4 ayloColorOut;
layout(location = 2) flat out uint ayloBlendOut;
#ifdef AYLO_INSTANCE_VALUES
layout(location = 3) flat out vec4 ayloInstanceOut[4];
#endif

const vec2 ayloCorners[6] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0),
                                    vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

#pragma aylo_shader

void main() {
    const vec2 ayloCorner = ayloCorners[gl_VertexIndex];
    const float ayloCos = cos(ayloRotation);
    const float ayloSin = sin(ayloRotation);
    // With Y down, this standard rotation turns clockwise on screen.
    VERTEX = ayloPosition + (mat2(ayloCos, ayloSin, -ayloSin, ayloCos) * ((ayloCorner * ayloSize) - ayloOrigin));
    UV = mix(ayloUvRect.xy, ayloUvRect.zw, ayloCorner);
    COLOR = ayloColor;
#pragma aylo_main
    gl_Position = ayloFrame.viewProjection * vec4(VERTEX, 0.0, 1.0);
    ayloUvOut = UV;
    ayloColorOut = vec4(COLOR.rgb * COLOR.a, COLOR.a);
    ayloBlendOut = ayloBlend;
#ifdef AYLO_INSTANCE_VALUES
    ayloInstanceOut = ayloInstance;
#endif
}
