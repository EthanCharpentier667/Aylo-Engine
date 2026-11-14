/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TimeCalls
*/

#pragma once

#include <cstdint>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/replication/Export.hpp"

namespace aylo::replication::timesync {
inline constexpr std::string_view kServerTimeCall =
    "aylo.net.serverTime";                                    ///< The client's estimate of the server clock.
inline constexpr std::string_view kRttCall = "aylo.net.rtt";  ///< The server's RTT to a client.

/// @brief The result of `aylo.net.serverTime`.
struct ServerTime {
    bool synced{false};  ///< Whether the server answered a ping since the connection: false while connecting.
    double time{0.0};    ///< The estimated server clock now, in seconds; 0 when not synced.
};
AYLO_ECS_CALL_DATA(ServerTime, "aylo.net.ServerTime", synced, time);

/// @brief The arguments of `aylo.net.rtt`.
struct RttArguments {
    std::uint32_t client{0};  ///< The client's id, as `net.Owner` and `net.ClientConnected` carry it.
};
AYLO_ECS_CALL_DATA(RttArguments, "aylo.net.RttArguments", client);

/// @brief The result of `aylo.net.rtt`.
struct Rtt {
    bool connected{false};  ///< Whether the client is connected; false for an unknown client.
    double rtt{0.0};        ///< The smoothed round-trip time, in seconds; 0 when not connected.
};
AYLO_ECS_CALL_DATA(Rtt, "aylo.net.Rtt", connected, rtt);

/// @brief Registers `aylo.net.serverTime` on @p app, on the `net.ClientState` resource. Called by the
/// ReplicationPlugin on the client side.
AYLO_REPLICATION_API void addClientTimeCalls(ecs::App& app);

/// @brief Registers `aylo.net.rtt` on @p app, on the `net.ServerState` resource. Called by the ReplicationPlugin on
/// the server side.
AYLO_REPLICATION_API void addServerTimeCalls(ecs::App& app);

/// @brief Registers both calls for a standalone session, where server and client are one: the clock is the one of the
/// game (`aylo.Time`) and the round trip is 0.
AYLO_REPLICATION_API void addStandaloneTimeCalls(ecs::App& app);
}  // namespace aylo::replication::timesync
