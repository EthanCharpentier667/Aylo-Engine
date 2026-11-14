/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Velocity3D
*/

#pragma once

#include <glm/ext/vector_float3.hpp>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::physics {

/// @brief How fast a 3D entity's body moves: the 3D family's Velocity, synced the same way.
/// @details Opt-in: gameplay writes it to set the speed, a dynamic body writes it back after each step.
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform3D{})
///     .add(aylo::plugins::physics::Collider3D{})
///     .add(aylo::plugins::physics::RigidBody{.type = aylo::plugins::physics::BodyType::kKinematic})
///     .add(aylo::plugins::physics::Velocity3D{.linear = {0.0F, 0.0F, -2.0F}});
/// @endcode
struct Velocity3D {
    glm::vec3 linear{0.0F};   ///< In metres per second, world axes (Y up).
    glm::vec3 angular{0.0F};  ///< In radians per second around each world axis, right-handed.

    friend bool operator==(const Velocity3D& first, const Velocity3D& second) = default;
};
AYLO_ECS_COMPONENT(Velocity3D, "aylo.Velocity3D", linear, angular);

}  // namespace aylo::plugins::physics
