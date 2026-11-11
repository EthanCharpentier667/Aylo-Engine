/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** shape.vert
*/

// The vertex stage of shapes: a ShapeInstance per instance, a quad of 6 vertices built from gl_VertexIndex covering the
// shape's bounding box plus a margin of a pixel and a half for the antialiased edge, turned around its center and seen
// through the frame's camera. UV is the position in the box, 0 to 1 (a little past them in the margin). COLOR is the
// fill colour. With no game code, it is the default shader of shapes.
layout(location = 0) in vec2 ayloCenter;            // World point the shape is centred on.
layout(location = 1) in vec2 ayloHalfSize;          // Half its bounding box, before rotation.
layout(location = 2) in float ayloRotation;         // Radians, clockwise on screen.
layout(location = 3) in float ayloRadius;           // Circle / capsule radius, or corner radius.
layout(location = 4) in float ayloOutlineThickness;
layout(location = 5) in uint ayloKindAndBlend;      // Low byte: 0 rect, 1 circle, 2 capsule. Next byte: blend mode.
layout(location = 6) in vec4 ayloFillColor;
layout(location = 7) in vec4 ayloOutlineColor;
#ifdef AYLO_INSTANCE_VALUES
layout(location = 8) in vec4 ayloInstance[4];  // The drawing's instance values.
#endif
layout(location = 0) out vec2 ayloUvOut;
layout(location = 1) out vec4 ayloColorOut;  // Premultiplied.
layout(location = 2) flat out uint ayloBlendOut;
#ifdef AYLO_INSTANCE_VALUES
layout(location = 3) flat out vec4 ayloInstanceOut[4];
#endif
layout(location = 7) out vec2 ayloLocalOut;  // Position in the shape's frame, in world units.
layout(location = 8) flat out vec4 ayloBoundsOut;  // Half size, radius, outline thickness.
layout(location = 9) flat out uint ayloKindOut;
layout(location = 10) flat out vec4 ayloOutlineColorOut;  // Premultiplied.

const vec2 ayloCorners[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                                    vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

#pragma aylo_shader

void main() {
    // The size of a pixel in world units, from the camera's scale: the margin keeps the antialiased edge inside the
    // quad whatever the zoom.
    const vec2 ayloPixel = 2.0 / (abs(vec2(ayloFrame.viewProjection[0][0], ayloFrame.viewProjection[1][1])) *
                                   ayloFrame.framebufferSize);
    const vec2 ayloLocal = ayloCorners[gl_VertexIndex] * (ayloHalfSize + (1.5 * max(ayloPixel.x, ayloPixel.y)));
    const float ayloCos = cos(ayloRotation);
    const float ayloSin = sin(ayloRotation);
    // With Y down, this standard rotation turns clockwise on screen.
    VERTEX = ayloCenter + (mat2(ayloCos, ayloSin, -ayloSin, ayloCos) * ayloLocal);
    UV = (ayloLocal / max(2.0 * ayloHalfSize, vec2(1e-5))) + 0.5;
    COLOR = ayloFillColor;
#pragma aylo_main
    gl_Position = ayloFrame.viewProjection * vec4(VERTEX, 0.0, 1.0);
    ayloUvOut = UV;
    ayloColorOut = vec4(COLOR.rgb * COLOR.a, COLOR.a);
    ayloBlendOut = (ayloKindAndBlend >> 8u) & 0xFFu;
#ifdef AYLO_INSTANCE_VALUES
    ayloInstanceOut = ayloInstance;
#endif
    ayloLocalOut = ayloLocal;
    ayloBoundsOut = vec4(ayloHalfSize, ayloRadius, ayloOutlineThickness);
    ayloKindOut = ayloKindAndBlend & 0xFFu;
    ayloOutlineColorOut = vec4(ayloOutlineColor.rgb * ayloOutlineColor.a, ayloOutlineColor.a);
}
