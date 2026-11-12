/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** screen.vert
*/

// The vertex stage of screen effects: one triangle that covers the whole target, built from gl_VertexIndex, no vertex
// buffer. VERTEX is in pixels of the target and UV runs 0..1 over it, both with (0, 0) at its top-left corner. A screen
// effect has no drawing to carry instance values: they keep their declared defaults, which the assembler declares as the
// global ayloInstance and fills at the start of main() (AYLO_INSTANCE_VALUES).
layout(location = 0) out vec2 ayloUvOut;
layout(location = 1) out vec4 ayloColorOut;
layout(location = 2) flat out uint ayloBlendOut;
#ifdef AYLO_INSTANCE_VALUES
layout(location = 3) flat out vec4 ayloInstanceOut[4];
#endif

#pragma aylo_shader

void main() {
    const vec2 ayloCorner = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
    VERTEX = ayloCorner * ayloFrame.framebufferSize;
    UV = ayloCorner;
    COLOR = vec4(1.0);
#pragma aylo_main
    gl_Position = vec4(((VERTEX / ayloFrame.framebufferSize) * 2.0) - 1.0, 0.0, 1.0);
    ayloUvOut = UV;
    ayloColorOut = COLOR;
    ayloBlendOut = 0u;
#ifdef AYLO_INSTANCE_VALUES
    ayloInstanceOut = ayloInstance;
#endif
}
