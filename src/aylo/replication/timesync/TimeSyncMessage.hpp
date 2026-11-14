/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TimeSyncMessage
*/

#pragma once

#include <cstddef>
#include <optional>
#include <span>

#include "aylo/network/Types.hpp"
#include "aylo/replication/Export.hpp"

namespace aylo::replication::timesync {
/// @brief The kTimePing message: "what time is it?", sent by a client.
struct TimePing {
    double clientTime{0.0};  ///< The client's local clock when it sent the ping, in seconds.
};

/// @brief The kTimePong message: the server's answer to a ping.
struct TimePong {
    double clientTime{0.0};  ///< The ping's clientTime, echoed untouched.
    double serverTime{0.0};  ///< The server's clock when it answered, in seconds.
};

/// @brief Writes a kTimePing message.
[[nodiscard]] AYLO_REPLICATION_API network::Bytes encodePing(const TimePing& ping);

/// @brief Writes a kTimePong message.
[[nodiscard]] AYLO_REPLICATION_API network::Bytes encodePong(const TimePong& pong);

/// @brief Reads a kTimePing message from the network; never throws.
/// @return The ping, or nothing when it is malformed or its time is not a finite number of seconds >= 0.
[[nodiscard]] AYLO_REPLICATION_API std::optional<TimePing> decodePing(std::span<const std::byte> message) noexcept;

/// @brief Reads a kTimePong message from the network; never throws.
/// @return The pong, or nothing when it is malformed or a time is not a finite number of seconds >= 0.
[[nodiscard]] AYLO_REPLICATION_API std::optional<TimePong> decodePong(std::span<const std::byte> message) noexcept;
}  // namespace aylo::replication::timesync
