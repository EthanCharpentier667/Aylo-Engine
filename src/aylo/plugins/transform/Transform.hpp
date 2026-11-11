/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Transform
*/

#pragma once

#include <glm/ext/vector_float2.hpp>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::transform {

/// @brief Where an entity is: what gameplay writes. Relative to its parent (ecs::Parent), or to the world without one.
/// @code
/// commands.spawn().add(aylo::plugins::transform::Transform{.position = {400.0F, 300.0F}});
/// @endcode
struct Transform {
    glm::vec2 position{0.0F};  ///< In world units, X right, Y down; in the parent's axes if it has one.
    float rotation = 0.0F;     ///< In radians, clockwise on screen.
    glm::vec2 scale{1.0F};     ///< 1: natural size; negative flips.
};
AYLO_ECS_COMPONENT(Transform, "rtype.Transform", position, rotation, scale);

/// @brief Where an entity is in the world: what rendering and collisions read, never written by gameplay.
/// @details Computed from Transform in PostUpdate by the TransformPlugin, which adds it to every entity with a
/// Transform. Equal to Transform without a parent; otherwise it combines the parents' transforms (combine()). Not
/// replicated: each process computes it.
struct GlobalTransform {
    glm::vec2 position{0.0F};  ///< In world units.
    float rotation = 0.0F;     ///< In radians, clockwise on screen.
    glm::vec2 scale{1.0F};     ///< Combined scale.
};
AYLO_ECS_COMPONENT(GlobalTransform, "rtype.GlobalTransform", position, rotation, scale);

}  // namespace aylo::plugins::transform
