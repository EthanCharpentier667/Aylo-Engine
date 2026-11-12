/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ClientInfo
*/

#pragma once

#include <cstdint>

#include "aylo/network/Types.hpp"
#include "aylo/network/io/Address.hpp"
#include "aylo/network/server/Session.hpp"

namespace aylo::network::server {
/// @brief A copy of what the server knows of one connected client, for tools (the admin channel, the editor).
struct ClientInfo {
    ClientId client{kNoClient};                    ///< The id given to the client.
    SessionState state{SessionState::kWelcoming};  ///< Welcoming or ready.
    io::Address address;                           ///< Where the client is.
    float rttMs{0.0F};                             ///< Smoothed round-trip time, in milliseconds.
    std::uint64_t droppedMessages{0};              ///< Unreliable messages dropped because they did not fit.
    std::uint64_t protocolErrors{0};               ///< Reliable messages dropped for an id beyond the window.
};
}  // namespace aylo::network::server
