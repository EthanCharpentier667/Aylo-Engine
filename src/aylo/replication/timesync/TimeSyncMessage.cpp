/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TimeSyncMessage
*/

#include "aylo/replication/timesync/TimeSyncMessage.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "aylo/network/Types.hpp"
#include "aylo/network/bytes/ByteReader.hpp"
#include "aylo/network/bytes/ByteWriter.hpp"
#include "aylo/replication/MessageType.hpp"

namespace aylo::replication::timesync {
namespace {
/// @return Whether a time of the wire is a usable number of seconds.
[[nodiscard]] bool isTime(const double seconds) noexcept { return std::isfinite(seconds) && seconds >= 0.0; }
}  // namespace

network::Bytes encodePing(const TimePing& ping) {
    network::Bytes bytes(1 + 8);
    network::bytes::ByteWriter writer{bytes};
    writer.writeU8(static_cast<std::uint8_t>(MessageType::kTimePing));
    writer.writeF64(ping.clientTime);
    return bytes;
}

network::Bytes encodePong(const TimePong& pong) {
    network::Bytes bytes(1 + 8 + 8);
    network::bytes::ByteWriter writer{bytes};
    writer.writeU8(static_cast<std::uint8_t>(MessageType::kTimePong));
    writer.writeF64(pong.clientTime);
    writer.writeF64(pong.serverTime);
    return bytes;
}

std::optional<TimePing> decodePing(const std::span<const std::byte> message) noexcept {
    network::bytes::ByteReader reader{message};
    if (reader.readU8() != static_cast<std::uint8_t>(MessageType::kTimePing)) {
        return std::nullopt;
    }
    const TimePing ping{.clientTime = reader.readF64()};
    if (!reader.isFinished() || !isTime(ping.clientTime)) {
        return std::nullopt;
    }
    return ping;
}

std::optional<TimePong> decodePong(const std::span<const std::byte> message) noexcept {
    network::bytes::ByteReader reader{message};
    if (reader.readU8() != static_cast<std::uint8_t>(MessageType::kTimePong)) {
        return std::nullopt;
    }
    const double clientTime = reader.readF64();
    const TimePong pong{.clientTime = clientTime, .serverTime = reader.readF64()};
    if (!reader.isFinished() || !isTime(pong.clientTime) || !isTime(pong.serverTime)) {
        return std::nullopt;
    }
    return pong;
}
}  // namespace aylo::replication::timesync
