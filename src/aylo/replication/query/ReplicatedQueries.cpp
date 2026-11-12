/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ReplicatedQueries
*/

#include "aylo/replication/query/ReplicatedQueries.hpp"

#include <optional>
#include <string_view>
#include <vector>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/query/QueryBuilder.hpp"
#include "aylo/ecs/system/QueryList.hpp"
#include "aylo/ecs/world/World.hpp"
#include "aylo/replication/exceptions/ReplicationExceptions.hpp"

namespace aylo::replication::query {
namespace {
/// @brief The name NetworkId is registered under (see component/NetworkId.hpp).
constexpr std::string_view kNetworkIdName = "net.NetworkId";
}  // namespace

std::vector<ecs::ComponentId> listReplicatedComponents(const ecs::World& world) {
    const auto replicated = world.getComponents().getRegistry().getReplicatedIds();
    return {replicated.begin(), replicated.end()};
}

ecs::QueryList makeReplicatedReads(const ecs::World& world) {
    ecs::QueryList list;
    for (const ecs::ComponentId id : listReplicatedComponents(world)) {
        list.queries.push_back(ecs::QueryBuilder{}.read(id));
    }
    return list;
}

ecs::QueryList makeUntrackedQueries(const ecs::World& world) {
    const std::optional<ecs::ComponentId> networkId = world.getComponents().find(kNetworkIdName);
    if (!networkId) {
        throw exceptions::ReplicationException{
            "The component 'net.NetworkId' is not registered: add the "
            "ReplicationPlugin before the server"};
    }
    ecs::QueryList list;
    for (const ecs::ComponentId id : listReplicatedComponents(world)) {
        list.queries.push_back(ecs::QueryBuilder{}.with(id).without(*networkId));
    }
    return list;
}
}  // namespace aylo::replication::query
