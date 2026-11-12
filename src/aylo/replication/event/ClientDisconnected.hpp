/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ClientDisconnected
*/

#pragma once

#include <cstdint>

#include "aylo/ecs/event/EventTraits.hpp"

namespace aylo::replication::event {
/// @brief Raised on the server when a client leaves for good, so the game can remove its ship with no network code.
struct ClientDisconnected {
    std::uint32_t client{0};  ///< The client that left (a network::ClientId).
    std::uint8_t reason{0};   ///< Why it left (a network::DisconnectReason).
};

AYLO_ECS_EVENT(ClientDisconnected, "net.ClientDisconnected", client, reason);
}  // namespace aylo::replication::event
