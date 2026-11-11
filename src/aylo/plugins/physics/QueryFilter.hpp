/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** QueryFilter
*/

#pragma once

#include <cstdint>
#include <optional>

#include "aylo/ecs/entity/Entity.hpp"
#include "plugins/physics/Collider.hpp"

namespace aylo::plugins::physics {

/// @brief Which bodies a scene query (raycast, shape cast, overlap) sees.
/// @details The mask only goes one way: the query picks the categories it sees, whatever the bodies' own masks (those
/// decide which bodies collide with each other, not what a query finds).
/// @code
/// const aylo::plugins::physics::QueryFilter enemiesAndWalls{.mask = kEnemy | kWall, .ignore = ship};
/// const auto hit = world->raycast(muzzle, {1.0F, 0.0F}, 800.0F, enemiesAndWalls);
/// @endcode
struct QueryFilter {
    std::uint32_t mask = kAllLayers;    ///< The collision categories it sees (Collider::layer).
    std::optional<ecs::Entity> ignore;  ///< An entity to skip: the shooter, for a laser fired from a ship.
    bool includeSensors = false;        ///< Whether sensors (triggers) are seen: no, by default.
};

}  // namespace aylo::plugins::physics
