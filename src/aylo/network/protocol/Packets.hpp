/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Packets
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>

#include "aylo/network/Types.hpp"
#include "aylo/network/bytes/Codec.hpp"
#include "aylo/network/crypto/Crypto.hpp"
#include "aylo/network/protocol/Constants.hpp"

namespace aylo::network::protocol {
/// @brief The first byte of a datagram: which transport packet follows. 0 is never valid.
enum class PacketKind : std::uint8_t {
    kConnectRequest = 0x01,
    kChallenge = 0x02,
    kChallengeResponse = 0x03,
    kAccepted = 0x04,
    kRejected = 0x05,
    kData = 0x06,
    kDisconnect = 0x07,
};

/// @brief Tells whether `kind` is a value of the wire (1 to 7, never 0).
[[nodiscard]] constexpr bool isValidWireValue(const PacketKind kind) noexcept {
    return kind >= PacketKind::kConnectRequest && kind <= PacketKind::kDisconnect;
}

/// @brief Size of the sealed resume request of a `ChallengeResponse`: the client (4), the token (8) and the tag.
inline constexpr std::size_t kSealedResumeSize = 4 + 8 + crypto::kTagSize;

/// @brief Size of the sealed body of `Accepted`: the session token (4), the client (4), the resume token (8), the tag.
inline constexpr std::size_t kSealedAcceptedSize = 4 + 4 + 8 + crypto::kTagSize;

/// @brief Size of the sealed body of `Disconnect`: the reason (1) and the tag.
inline constexpr std::size_t kSealedDisconnectSize = 1 + crypto::kTagSize;

/// @brief Client to server: "I want to connect". Padded to `kHandshakePadding` so that no answer is bigger.
struct ConnectRequest {
    static constexpr auto kKind = PacketKind::kConnectRequest;  ///< The kind byte.
    std::uint16_t version{0};                                   ///< The protocol version of the client.

    /// @brief Lists the fields in wire order; a wrong protocol id makes the stream invalid.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.constant(kProtocolId);
        stream.value(self.version);
        stream.padTo(kHandshakePadding);
    }
};

/// @brief Server to client: proves the client owns its address, since only the real address receives it, and gives
/// the server's keys: its identity and its key exchange key, signed by the identity.
struct Challenge {
    static constexpr auto kKind = PacketKind::kChallenge;  ///< The kind byte.
    std::uint64_t challenge{0};                            ///< The random value to echo.
    crypto::Key identity{};                                ///< The server's Ed25519 public key.
    crypto::Key exchange{};                                ///< The server's X25519 public key for this run.
    crypto::Signature signature{};                         ///< `identity`'s signature of the context, then `exchange`.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.challenge);
        stream.bytes(self.identity);
        stream.bytes(self.exchange);
        stream.bytes(self.signature);
    }
};

/// @brief Client to server: echoes the challenge and gives the client's key exchange key, with the session to resume
/// sealed under the new keys. Padded to `kHandshakePadding`.
struct ChallengeResponse {
    static constexpr auto kKind = PacketKind::kChallengeResponse;  ///< The kind byte.
    std::uint64_t challenge{0};                                    ///< The echoed value.
    crypto::Key exchange{};                                        ///< The client's X25519 public key.
    std::array<std::byte, kSealedResumeSize> sealedResume{};       ///< The client and token to resume, sealed.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.challenge);
        stream.bytes(self.exchange);
        stream.bytes(self.sealedResume);
        stream.padTo(kHandshakePadding);
    }
};

/// @brief Server to client: the session exists. Its tokens and the client id are sealed under the session keys.
struct Accepted {
    static constexpr auto kKind = PacketKind::kAccepted;  ///< The kind byte.
    std::array<std::byte, kSealedAcceptedSize> sealed{};  ///< Session token, client, resume token, sealed.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.bytes(self.sealed);
    }
};

/// @brief Server to client: why the connection is refused.
struct Rejected {
    static constexpr auto kKind = PacketKind::kRejected;  ///< The kind byte.
    RejectReason reason{RejectReason::kVersionMismatch};  ///< Why.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.reason);
    }
};

/// @brief Both ways: everything once connected. Only the session token and the counter are in clear; `sealed` holds
/// the sequence, the acknowledgements and the messages, and points into the datagram, so it is only valid while the
/// received buffer lives.
struct Data {
    static constexpr auto kKind = PacketKind::kData;  ///< The kind byte.
    std::uint32_t sessionToken{0};                    ///< The token given by `Accepted`.
    std::uint64_t counter{0};                         ///< The nonce of the seal, never twice in one direction.
    std::span<const std::byte> sealed;                ///< The sealed body, up to the end of the datagram.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.sessionToken);
        stream.value(self.counter);
        stream.rest(self.sealed);
    }
};

/// @brief Both ways: closes the connection. The reason is sealed, so only the peer can close it.
struct Disconnect {
    static constexpr auto kKind = PacketKind::kDisconnect;  ///< The kind byte.
    std::uint32_t sessionToken{0};                          ///< The token of the session.
    std::uint64_t counter{0};                               ///< The nonce of the seal, from the same count as `Data`.
    std::array<std::byte, kSealedDisconnectSize> sealed{};  ///< The reason, sealed.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.sessionToken);
        stream.value(self.counter);
        stream.bytes(self.sealed);
    }
};

/// @brief Any transport packet.
using Packet = std::variant<ConnectRequest, Challenge, ChallengeResponse, Accepted, Rejected, Data, Disconnect>;

/// @brief The codec of the transport packets.
using PacketCodec = bytes::Codec<PacketKind, Packet>;
}  // namespace aylo::network::protocol
