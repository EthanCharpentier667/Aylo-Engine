/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ClientConnected
*/

#pragma once

#include <cstdint>

#include "aylo/ecs/event/EventTraits.hpp"

namespace aylo::replication::event {
/// @brief Raised on the server when a client joins the game, so the game can spawn its ship with no network code.
struct ClientConnected {
    std::uint32_t client{0};  ///< The client that joined (a network::ClientId).
};

AYLO_ECS_EVENT(ClientConnected, "net.ClientConnected", client);
}  // namespace aylo::replication::event
