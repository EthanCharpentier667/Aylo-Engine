/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StateMessage
*/

#include "aylo/replication/state/StateMessage.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <span>
#include <string_view>

#include "aylo/network/Types.hpp"
#include "aylo/network/bytes/ByteReader.hpp"
#include "aylo/network/bytes/ByteWriter.hpp"
#include "aylo/replication/MessageType.hpp"

namespace aylo::replication::state {
namespace {
/// @brief The longest name or value: its length is one byte.
constexpr std::size_t kMaxLength = 255;

/// @return Whether a text fits a `string8`, and is not empty.
[[nodiscard]] bool fits(const std::string_view text) noexcept { return !text.empty() && text.size() <= kMaxLength; }
}  // namespace

std::optional<network::Bytes> encodeState(const std::string_view machine, const std::string_view value) {
    if (!fits(machine) || !fits(value)) {
        return std::nullopt;
    }
    network::Bytes bytes(1 + 1 + machine.size() + 1 + value.size());
    network::bytes::ByteWriter writer{bytes};
    writer.writeU8(static_cast<std::uint8_t>(MessageType::kState));
    writer.writeString(machine);
    writer.writeString(value);
    return bytes;
}

std::optional<StateMessage> decodeState(const std::span<const std::byte> message) noexcept {
    try {
        network::bytes::ByteReader reader{message};
        if (reader.readU8() != static_cast<std::uint8_t>(MessageType::kState)) {
            return std::nullopt;
        }
        StateMessage result{.machine = reader.readString(), .value = reader.readString()};
        if (!reader.isValid() || reader.remainingSize() != 0 || result.machine.empty() || result.value.empty()) {
            return std::nullopt;
        }
        return result;
    } catch (const std::exception&) {  // an allocation that fails: the message is dropped like any bad one
        return std::nullopt;
    }
}
}  // namespace aylo::replication::state
