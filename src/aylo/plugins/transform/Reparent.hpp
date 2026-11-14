/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Reparent
*/

#pragma once

#include "Transform.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "plugins/transform/Export.hpp"

namespace aylo::plugins::transform {
/// @brief Records making @p child a root without moving it: `remove<ecs::Parent>()` alone would leave a Transform2D
/// relative to a parent it no longer has. @p global comes from a query, as of the last PostUpdate.
/// @code
/// aylo::plugins::transform::detachInPlace(commands, turret, global);  // the turret falls off the boss, where it was
/// @endcode
AYLO_PLUGIN_TRANSFORM_API void detachInPlace(ecs::Commands& commands, ecs::Entity child,
                                             const GlobalTransform2D& global);

/// @brief Records making @p child a child of @p parent without moving it, its Transform2D becoming relative to it.
AYLO_PLUGIN_TRANSFORM_API void attachInPlace(ecs::Commands& commands, ecs::Entity child,
                                             const GlobalTransform2D& global, ecs::Entity parent,
                                             const GlobalTransform2D& parentGlobal);
}  // namespace aylo::plugins::transform
