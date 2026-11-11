/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** particle.vert
*/

// The vertex stage of particles: a ParticleInstance per instance, a unit quad centred on it, turned (along its
// velocity for the velocity alignment, stretched by its speed), showing its flipbook frame. With no game code, it is
// the default shader of particles.
layout(location = 0) in vec4 ayloColor;      // Multiplies the texture.
layout(location = 1) in vec2 ayloPosition;   // Centre of the particle, in world units.
layout(location = 2) in vec2 ayloSize;       // In world units.
layout(location = 3) in float ayloRotation;  // Radians, clockwise on screen.
layout(location = 4) in vec2 ayloVelocity;   // World units per second.
layout(location = 5) in float ayloLife;      // 0 at birth, 1 at death.
layout(location = 0) out vec2 ayloUvOut;
layout(location = 1) out vec4 ayloColorOut;
layout(location = 2) flat out uint ayloBlendOut;

const vec2 ayloCorners[6] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0),
                                    vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

#pragma aylo_shader

void main() {
    const vec2 ayloCorner = ayloCorners[gl_VertexIndex];
    float ayloAngle = ayloRotation;
    vec2 ayloQuadSize = ayloSize;
    const float ayloSpeed = length(ayloVelocity);
    if (ayloParticles.alignment == 1u && ayloSpeed > 0.0) {
        ayloAngle = atan(ayloVelocity.y, ayloVelocity.x);
        ayloQuadSize.x += ayloSpeed * ayloParticles.stretch;
    }
    const float ayloCos = cos(ayloAngle);
    const float ayloSin = sin(ayloAngle);
    VERTEX = ayloPosition + (mat2(ayloCos, ayloSin, -ayloSin, ayloCos) * ((ayloCorner - 0.5) * ayloQuadSize));
    // The flipbook frame of this moment of the life, a cell of the uv rect: left to right, then top to bottom.
    const uint ayloFrameIndex = uint(clamp(ayloLife, 0.0, 0.9999) * ayloParticles.cycles *
                                      float(ayloParticles.frameCount)) % ayloParticles.frameCount;
    const vec2 ayloCellSize = (ayloParticles.uvRect.zw - ayloParticles.uvRect.xy) /
                               vec2(ayloParticles.columns, ayloParticles.rows);
    const vec2 ayloCellMin = ayloParticles.uvRect.xy +
        (vec2(ayloFrameIndex % ayloParticles.columns, ayloFrameIndex / ayloParticles.columns) * ayloCellSize);
    UV = ayloCellMin + (ayloCorner * ayloCellSize);
    COLOR = ayloColor;
#pragma aylo_main
    gl_Position = ayloFrame.viewProjection * vec4(VERTEX, 0.0, 1.0);
    ayloUvOut = UV;
    ayloColorOut = vec4(COLOR.rgb * COLOR.a, COLOR.a);
    ayloBlendOut = ayloParticles.blend;
}
