/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Handshake
*/

#include "aylo/network/protocol/Handshake.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "aylo/network/Types.hpp"
#include "aylo/network/bytes/ByteReader.hpp"
#include "aylo/network/bytes/ByteWriter.hpp"
#include "aylo/network/crypto/Crypto.hpp"
#include "aylo/network/protocol/Constants.hpp"
#include "aylo/network/protocol/Packets.hpp"

namespace aylo::network::protocol {
namespace {
/// @brief The counter of the handshake's seals, in each direction.
constexpr std::uint64_t kHandshakeCounter = 0;

/// @return The kind byte of `Accepted`: the additional data of its seal.
std::array<std::byte, 1> acceptedAdditional() noexcept { return {static_cast<std::byte>(PacketKind::kAccepted)}; }

/// @return The challenge, lowest byte first: the additional data of the resume request's seal.
std::array<std::byte, sizeof(std::uint64_t)> challengeAdditional(const std::uint64_t challenge) noexcept {
    std::array<std::byte, sizeof(std::uint64_t)> bytes{};
    bytes::ByteWriter writer{bytes};
    writer.writeU64(challenge);
    return bytes;
}

/// @return @p sealed copied into an array of its exact size.
template <std::size_t Size>
std::array<std::byte, Size> toArray(const Bytes& sealed) {
    std::array<std::byte, Size> result{};
    std::ranges::copy_n(sealed.begin(), static_cast<std::ptrdiff_t>(std::min(sealed.size(), Size)), result.begin());
    return result;
}
}  // namespace

std::array<std::byte, kSealedAcceptedSize> sealAccepted(const crypto::Key& key, const AcceptedBody& body) {
    std::array<std::byte, kSealedAcceptedSize - crypto::kTagSize> plaintext{};
    bytes::ByteWriter writer{plaintext};
    writer.writeU32(body.sessionToken);
    writer.writeU32(body.client);
    writer.writeU64(body.resumeToken);
    return toArray<kSealedAcceptedSize>(crypto::seal(key, kHandshakeCounter, acceptedAdditional(), plaintext));
}

std::optional<AcceptedBody> openAccepted(const crypto::Key& key, const Accepted& accepted) {
    const auto plaintext = crypto::open(key, kHandshakeCounter, acceptedAdditional(), accepted.sealed);
    if (!plaintext) {
        return std::nullopt;
    }
    bytes::ByteReader reader{*plaintext};
    AcceptedBody body;
    body.sessionToken = reader.readU32();
    body.client = reader.readU32();
    body.resumeToken = reader.readU64();
    if (!reader.isValid() || !reader.isFinished()) {
        return std::nullopt;
    }
    return body;
}

std::array<std::byte, kSealedResumeSize> sealResume(const crypto::Key& key, const std::uint64_t challenge,
                                                    const ResumeRequest& request) {
    std::array<std::byte, kSealedResumeSize - crypto::kTagSize> plaintext{};
    bytes::ByteWriter writer{plaintext};
    writer.writeU32(request.client);
    writer.writeU64(request.token);
    return toArray<kSealedResumeSize>(crypto::seal(key, kHandshakeCounter, challengeAdditional(challenge), plaintext));
}

std::optional<ResumeRequest> openResume(const crypto::Key& key, const ChallengeResponse& response) {
    const auto plaintext =
        crypto::open(key, kHandshakeCounter, challengeAdditional(response.challenge), response.sealedResume);
    if (!plaintext) {
        return std::nullopt;
    }
    bytes::ByteReader reader{*plaintext};
    ResumeRequest request;
    request.client = reader.readU32();
    request.token = reader.readU64();
    if (!reader.isValid() || !reader.isFinished()) {
        return std::nullopt;
    }
    return request;
}

Bytes exchangeSignedMessage(const crypto::Key& exchange) {
    Bytes message;
    message.reserve(kExchangeSignatureContext.size() + exchange.size());
    for (const char character : kExchangeSignatureContext) {
        message.push_back(static_cast<std::byte>(character));
    }
    message.insert(message.end(), exchange.begin(), exchange.end());
    return message;
}
}  // namespace aylo::network::protocol
