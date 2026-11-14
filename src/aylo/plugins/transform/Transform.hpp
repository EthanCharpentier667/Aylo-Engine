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
/// commands.spawn().add(aylo::plugins::transform::Transform2D{.position = {400.0F, 300.0F}});
/// @endcode
struct Transform2D {
    glm::vec2 position{0.0F};  ///< In world units, X right, Y down; in the parent's axes if it has one.
    float rotation = 0.0F;     ///< In radians, clockwise on screen.
    glm::vec2 scale{1.0F};     ///< 1: natural size; negative flips.
};
AYLO_ECS_COMPONENT(Transform2D, "aylo.Transform2D", position, rotation, scale);

/// @brief Where an entity is in the world: what rendering and collisions read, never written by gameplay.
/// @details Computed from Transform2D in PostUpdate by the TransformPlugin, which adds it to every entity with a
/// Transform2D. Equal to Transform2D without a parent; otherwise it combines the parents' transforms (combine()). Not
/// replicated: each process computes it.
struct GlobalTransform2D {
    glm::vec2 position{0.0F};  ///< In world units.
    float rotation = 0.0F;     ///< In radians, clockwise on screen.
    glm::vec2 scale{1.0F};     ///< Combined scale.
};
AYLO_ECS_COMPONENT(GlobalTransform2D, "aylo.GlobalTransform2D", position, rotation, scale);

/// @brief Draws an entity between its last two fixed ticks, so what moves in FixedUpdate looks smooth whatever the
/// frame rate.
/// @details Add it next to the Transform2D of an entity moved in FixedUpdate (by gameplay or by the physics). On the
/// client (and standalone), the TransformPlugin records the Transform2D after each fixed tick and computes the
/// GlobalTransform2D from the pose blended between the last two ticks by Time::alpha: what is drawn is at most one tick
/// behind the simulation. A Transform2D changed when no tick ran (a teleport in Update, the editor) is drawn where it is
/// at once. Only GlobalTransform2D is blended: Transform2D, the physics and the server keep the simulated pose. The members
/// are the plugin's state, not reflected: a script sees a component without fields.
/// @code
/// commands.spawn().add(aylo::plugins::transform::Transform2D{}).add(aylo::plugins::transform::TransformInterpolation{});
/// @endcode
struct TransformInterpolation {
    Transform2D previous;      ///< The pose recorded after the tick before the last one.
    Transform2D current;       ///< The pose recorded after the last tick.
    Transform2D drawn;         ///< `previous` blended toward `current` by Time::alpha: what GlobalTransform2D starts from.
    std::uint32_t tick = 0;  ///< Time::tick when `current` was recorded.
    bool recorded = false;   ///< Whether a pose was recorded yet; until then the Transform2D is drawn as it is.
};
AYLO_ECS_COMPONENT(TransformInterpolation, "aylo.TransformInterpolation");

}  // namespace aylo::plugins::transform
