/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** NetworkSettings
*/

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "aylo/network/crypto/Crypto.hpp"

namespace aylo::network {

/// @brief The tunable values of the transport, chosen per deployment; the defaults are the design's.
struct NetworkSettings {
    /// @brief UDP port the server listens on.
    std::uint16_t port{4242};
    /// @brief Maximum number of connected clients.
    std::uint32_t maxClients{64};
    /// @brief Silence after which a connection is considered lost.
    std::chrono::milliseconds timeout{2000};
    /// @brief Interval after which an idle connection sends a keepalive.
    std::chrono::milliseconds keepalive{100};
    /// @brief Minimum delay before a reliable message is sent again.
    std::chrono::milliseconds minResendInterval{50};
    /// @brief Round-trip time assumed before the first measure.
    std::chrono::milliseconds initialRtt{100};
    /// @brief Delay between two sends of a handshake request.
    std::chrono::milliseconds handshakeRetry{100};
    /// @brief Time after which a client gives up connecting.
    std::chrono::milliseconds handshakeTimeout{5000};
    /// @brief Time a server keeps a challenge before forgetting it.
    std::chrono::milliseconds challengeExpiry{5000};
    /// @brief Maximum number of challenges a server keeps at once.
    std::size_t maxPendingChallenges{256};
    /// @brief Server: the seed of its identity (Ed25519). The same seed always gives the same public key, which
    /// clients may pin; none: a new identity every run.
    std::optional<crypto::Key> identitySeed;
    /// @brief Client: the public key of the identity the server must prove. None: any server is accepted; the traffic
    /// is still encrypted, but a machine on the path could pose as the server.
    std::optional<crypto::Key> trustedServerKey;
};

}  // namespace aylo::network
