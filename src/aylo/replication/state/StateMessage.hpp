/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StateMessage
*/

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "aylo/network/Types.hpp"
#include "aylo/replication/Export.hpp"

namespace aylo::replication::state {
/// @brief The kState message: the value a state machine the server owns is in (ecs::States::Replication::kFromServer).
struct StateMessage {
    std::string machine;  ///< The machine's name, 1 to 255 bytes.
    std::string value;    ///< Its value, 1 to 255 bytes.
};

/// @brief Writes a kState message.
/// @return The bytes, or nothing when the name or the value is empty or longer than 255 bytes.
[[nodiscard]] AYLO_REPLICATION_API std::optional<network::Bytes> encodeState(std::string_view machine,
                                                                             std::string_view value);

/// @brief Reads a kState message from the network.
/// @return The message, or nothing when it is not a well-formed kState message: never throws.
[[nodiscard]] AYLO_REPLICATION_API std::optional<StateMessage> decodeState(std::span<const std::byte> message) noexcept;
}  // namespace aylo::replication::state
