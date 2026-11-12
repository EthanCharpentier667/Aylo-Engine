/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Transform
*/

#pragma once

#include <cstdint>
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
AYLO_ECS_COMPONENT(Transform, "aylo.Transform", position, rotation, scale);

/// @brief Where an entity is in the world: what rendering and collisions read, never written by gameplay.
/// @details Computed from Transform in PostUpdate by the TransformPlugin, which adds it to every entity with a
/// Transform. Equal to Transform without a parent; otherwise it combines the parents' transforms (combine()). Not
/// replicated: each process computes it.
struct GlobalTransform {
    glm::vec2 position{0.0F};  ///< In world units.
    float rotation = 0.0F;     ///< In radians, clockwise on screen.
    glm::vec2 scale{1.0F};     ///< Combined scale.
};
AYLO_ECS_COMPONENT(GlobalTransform, "aylo.GlobalTransform", position, rotation, scale);

/// @brief Draws an entity between its last two fixed ticks, so what moves in FixedUpdate looks smooth whatever the
/// frame rate.
/// @details Add it next to the Transform of an entity moved in FixedUpdate (by gameplay or by the physics). On the
/// client (and standalone), the TransformPlugin records the Transform after each fixed tick and computes the
/// GlobalTransform from the pose blended between the last two ticks by Time::alpha: what is drawn is at most one tick
/// behind the simulation. A Transform changed when no tick ran (a teleport in Update, the editor) is drawn where it is
/// at once. Only GlobalTransform is blended: Transform, the physics and the server keep the simulated pose. The members
/// are the plugin's state, not reflected: a script sees a component without fields.
/// @code
/// commands.spawn().add(aylo::plugins::transform::Transform{}).add(aylo::plugins::transform::TransformInterpolation{});
/// @endcode
struct TransformInterpolation {
    Transform previous;      ///< The pose recorded after the tick before the last one.
    Transform current;       ///< The pose recorded after the last tick.
    Transform drawn;         ///< `previous` blended toward `current` by Time::alpha: what GlobalTransform starts from.
    std::uint32_t tick = 0;  ///< Time::tick when `current` was recorded.
    bool recorded = false;   ///< Whether a pose was recorded yet; until then the Transform is drawn as it is.
};
AYLO_ECS_COMPONENT(TransformInterpolation, "aylo.TransformInterpolation");

}  // namespace aylo::plugins::transform
