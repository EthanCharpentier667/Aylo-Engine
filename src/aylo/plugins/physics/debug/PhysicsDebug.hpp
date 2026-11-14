/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PhysicsDebug
*/

#pragma once

#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::physics::debug {

/// @brief Draws an entity's physics on the client: add it to see its collider and its speed, as gizmos in Unity.
/// @details Read by the PhysicsDebugPlugin, which does all the drawing. A separate component rather than a field of
/// Collider or RigidBody: changing those rebuilds the body and restarts its contacts, while adding or removing this one
/// never touches the simulation.
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform2D{.position = {100.0F, 200.0F}})
///     .add(aylo::plugins::physics::Collider{.shape = aylo::plugins::physics::ShapeType::kCircle, .radius = 8.0F})
///     .add(aylo::plugins::physics::RigidBody{})
///     .add(aylo::plugins::physics::debug::PhysicsDebug{});
/// @endcode
struct PhysicsDebug {
    bool shape = true;                                        ///< Draws the Collider's outline.
    bool velocity = true;                                     ///< Draws the Velocity as an arrow.
    contract::renderer::Color color{0.2F, 1.0F, 0.4F, 1.0F};  ///< The outline's and arrow's colour.
};
AYLO_ECS_COMPONENT(PhysicsDebug, "aylo.PhysicsDebug", shape, velocity, color);

}  // namespace aylo::plugins::physics::debug
