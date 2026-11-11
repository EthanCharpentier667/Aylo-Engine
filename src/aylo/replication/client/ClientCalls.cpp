/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ClientCalls
*/

#include "aylo/replication/client/ClientCalls.hpp"

#include <cstdint>
#include <optional>
#include <string>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/network/Types.hpp"
#include "aylo/replication/client/ClientState.hpp"

namespace aylo::replication::client {
namespace {
/// @brief `rtype.replication.localClient`: this client's id, once the server gave it.
LocalClient localClient(ecs::Res<ClientState> state) {
    const std::optional<network::ClientId> id = state->clientId();
    return LocalClient{.connected = id.has_value(), .client = id.value_or(std::uint32_t{0})};
}
}  // namespace

void addClientCalls(ecs::App& app) { app.addCall(std::string{kLocalClientCall}, &localClient); }
}  // namespace aylo::replication::client
