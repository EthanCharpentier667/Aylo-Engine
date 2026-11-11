/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ComponentOptions
*/

#pragma once

#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"

namespace aylo::plugins::physics {

/// @brief How Collider is registered: replicated whenever it changes, so clients build the bodies they query (scene
/// queries, predicted movement) and draw (debug view) without simulating.
/// @details Every plugin that registers Collider uses it: a component registered twice with other options is refused.
/// Only ECS options: the physics knows nothing of the network, which reads them generically.
inline constexpr ecs::ComponentOptions kColliderOptions{.replication = ecs::ReplicationMode::kEveryChange};

/// @brief How RigidBody is registered: replicated whenever it changes, for the same reason as Collider.
inline constexpr ecs::ComponentOptions kRigidBodyOptions{.replication = ecs::ReplicationMode::kEveryChange};

}  // namespace aylo::plugins::physics
