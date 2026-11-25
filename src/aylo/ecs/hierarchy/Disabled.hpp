/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Disabled
*/

#pragma once

#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"

namespace aylo::ecs {
/// @brief Switches an entity off, its children with it, without despawning it: the queries with an Enabled filter
/// (the drawing, physics and audio systems, and every script query) skip it and its descendants. Add and remove it
/// like any tag. Replicated: a client hides what the server disabled.
struct Disabled {};
AYLO_ECS_COMPONENT(Disabled, "aylo.Disabled");

/// @brief The options Disabled is always registered with, whoever does it first.
inline constexpr ComponentOptions kDisabledOptions{.presence = Presence::kBoth,
                                                   .replication = ReplicationMode::kEveryChange};

/// @brief Disabled is registered with kDisabledOptions when no options are given.
template <>
inline constexpr ComponentOptions kDefaultComponentOptions<Disabled> = kDisabledOptions;
}  // namespace aylo::ecs
