/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Lines3D
*/

#pragma once

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <vector>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::graphics {

/// @brief The component ("aylo.Lines3D") of a set of segments in the 3D world, drawn each frame at the entity's
/// GlobalTransform3D: a grid, the outline of an area, a debug drawing.
/// @details The points go in pairs, each pair one segment, in the entity's axes. Drawn by a renderer that supports 3D,
/// in a frame with a Camera3D, one pixel wide, hidden behind what is nearer.
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform3D{})
///     .add(aylo::plugins::graphics::Lines3D{.points = {{0, 0, 0}, {1, 0, 0}}, .color = {1, 0, 0, 1}});
/// @endcode
struct Lines3D {
    std::vector<glm::vec3> points;  ///< Two per segment.
    glm::vec4 color{1.0F};          ///< Linear RGBA, not premultiplied.
};

AYLO_ECS_COMPONENT(Lines3D, "aylo.Lines3D", points, color);
}  // namespace aylo::plugins::graphics
