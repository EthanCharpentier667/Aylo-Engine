/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ReplicatedQueries
*/

#pragma once

#include <vector>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/system/QueryList.hpp"
#include "aylo/replication/Export.hpp"

namespace aylo::ecs {
class World;
}  // namespace aylo::ecs

/// @brief The queries through which the server reads the game's components, which no typed Query can name: they are
/// given to a system as the QueryList of a DynamicQueries parameter, one query per replicated component, in the order
/// of listReplicatedComponents().
namespace aylo::replication::query {
/// @param world A world whose replicated components are all registered.
/// @return The replicated components, in the order the queries below follow.
[[nodiscard]] AYLO_REPLICATION_API std::vector<ecs::ComponentId> listReplicatedComponents(const ecs::World& world);

/// @brief One query per replicated component that reads its value: what a system needs to write snapshots. The
/// system reads every replicated component, so a game system that writes one in the same phase has to be ordered
/// against it.
/// @param world A world whose replicated components are all registered.
/// @return The queries.
[[nodiscard]] AYLO_REPLICATION_API ecs::QueryList makeReplicatedReads(const ecs::World& world);

/// @brief One query per replicated component that finds the entities holding it and no NetworkId: what a system needs
/// to give them one. It reads no value and declares no access, so it conflicts with no game system.
/// @param world A world whose replicated components and NetworkId are registered.
/// @return The queries.
/// @throws exceptions::ReplicationException When NetworkId is not registered.
[[nodiscard]] AYLO_REPLICATION_API ecs::QueryList makeUntrackedQueries(const ecs::World& world);
}  // namespace aylo::replication::query
