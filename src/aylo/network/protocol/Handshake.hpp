/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Handshake
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "aylo/network/Export.hpp"
#include "aylo/network/Types.hpp"
#include "aylo/network/crypto/Crypto.hpp"
#include "aylo/network/protocol/Packets.hpp"

/// The sealed parts of the handshake, sealed and opened the same way by the client and the server (protocol.md,
/// section 4). Each is sealed with counter 0 of its direction: `Data` starts at 1.
namespace aylo::network::protocol {
/// @brief What `Accepted` seals.
struct AcceptedBody {
    std::uint32_t sessionToken{0};  ///< The token of every later datagram.
    ClientId client{kNoClient};     ///< The id given to the client.
    std::uint64_t resumeToken{0};   ///< The token to resume the session later.
};

/// @brief What `ChallengeResponse` seals: the session the client wants to resume, `kNoClient` for a new one.
struct ResumeRequest {
    ClientId client{kNoClient};  ///< The client to resume.
    std::uint64_t token{0};      ///< Its resume token.
};

/// @return @p body sealed by the server with @p key (its transmit key), the kind byte as additional data.
[[nodiscard]] AYLO_NETWORK_API std::array<std::byte, kSealedAcceptedSize> sealAccepted(const crypto::Key& key,
                                                                                       const AcceptedBody& body);

/// @return What @p accepted seals, opened by the client with @p key (its receive key), or nothing if it does not open.
[[nodiscard]] AYLO_NETWORK_API std::optional<AcceptedBody> openAccepted(const crypto::Key& key,
                                                                        const Accepted& accepted);

/// @return @p request sealed by the client with @p key (its transmit key), the challenge as additional data.
[[nodiscard]] AYLO_NETWORK_API std::array<std::byte, kSealedResumeSize> sealResume(const crypto::Key& key,
                                                                                   std::uint64_t challenge,
                                                                                   const ResumeRequest& request);

/// @return What @p response seals, opened by the server with @p key (its receive key), or nothing if it does not open:
/// the client does not hold the key it sent.
[[nodiscard]] AYLO_NETWORK_API std::optional<ResumeRequest> openResume(const crypto::Key& key,
                                                                       const ChallengeResponse& response);

/// @return What the server's identity signs: `kExchangeSignatureContext`, then @p exchange.
[[nodiscard]] AYLO_NETWORK_API Bytes exchangeSignedMessage(const crypto::Key& exchange);
}  // namespace aylo::network::protocol
