/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Constants
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace aylo::network::protocol {

/// @brief Identifies AYLO datagrams ("AYLO"); anything else is dropped.
inline constexpr std::uint32_t kProtocolId = 0x41594C4F;

/// @brief Bumped by every change of a wire layout or of a constant below.
inline constexpr std::uint16_t kProtocolVersion = 2;

/// @brief Largest datagram, in bytes, that fits a typical path MTU.
inline constexpr std::size_t kMaxPacketSize = 1200;

/// @brief Largest message payload, in bytes, that fits a datagram with its headers.
inline constexpr std::size_t kMaxMessageSize = 1000;

/// @brief Size of the handshake requests, padded so that no answer is larger: the server cannot amplify a flood.
inline constexpr std::size_t kHandshakePadding = 160;

/// @brief What the server signs to prove its key exchange key is its own, before the key itself.
inline constexpr std::string_view kExchangeSignatureContext = "AYLO key exchange v2";

/// @brief Number of reliable messages that may be in flight at once.
inline constexpr std::size_t kReliableWindow = 256;

/// @brief Number of reliable messages that may wait to be sent before the connection overflows.
inline constexpr std::size_t kMaxPendingReliable = 1024;

/// @brief Number of datagrams a connection may send in one flush.
inline constexpr std::size_t kMaxDatagramsPerFlush = 8;

/// @brief Number of sent datagrams remembered to match their acknowledgements.
inline constexpr std::size_t kSentPacketHistory = 1024;

}  // namespace aylo::network::protocol
