/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Velocity
*/

#pragma once

#include <glm/ext/vector_float2.hpp>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::physics {

/// @brief How fast an entity's body moves: gameplay writes it to set the speed, the physics plugin writes it back.
/// @details Opt-in: only the entities that have one are synced. Before each step, a Velocity that gameplay changed is
/// given to the body (a dynamic or kinematic one; a static body ignores it); after the step, a dynamic body writes its
/// velocity back, so gravity, damping, impulses and collisions show up in it. A kinematic body only reads it: it moves
/// at its Velocity, except on the ticks gameplay moves its Transform, where it moves to the Transform instead.
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform{.position = {0.0F, 200.0F}})
///     .add(aylo::plugins::physics::RigidBody{.type = aylo::plugins::physics::BodyType::kKinematic})
///     .add(aylo::plugins::physics::Collider{.shape = aylo::plugins::physics::ShapeType::kCircle, .radius = 4.0F})
///     .add(aylo::plugins::physics::Velocity{.linear = {300.0F, 0.0F}});
/// @endcode
struct Velocity {
    glm::vec2 linear{0.0F};  ///< In world units per second, Y down.
    float angular = 0.0F;    ///< In radians per second, clockwise on screen.

    friend bool operator==(const Velocity& first, const Velocity& second) = default;
};
AYLO_ECS_COMPONENT(Velocity, "aylo.Velocity", linear, angular);

}  // namespace aylo::plugins::physics
