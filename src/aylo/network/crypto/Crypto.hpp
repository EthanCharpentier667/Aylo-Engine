/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Crypto
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "aylo/network/Export.hpp"
#include "aylo/network/RandomSource.hpp"
#include "aylo/network/Types.hpp"

/// The cryptography of the transport: key exchange, server identity, authenticated encryption of datagrams. A wrapper
/// of libsodium, which stays private to this folder's sources (docs/network/protocol.md, section 4 and 5).
namespace aylo::network::crypto {
inline constexpr std::size_t kKeySize = 32;        ///< An X25519 or Ed25519 public key, a session key, a seed.
inline constexpr std::size_t kSignatureSize = 64;  ///< An Ed25519 signature.
inline constexpr std::size_t kTagSize = 16;        ///< What sealing adds to a plaintext (the Poly1305 tag).

using Key = std::array<std::byte, kKeySize>;              ///< A public key, a session key or a seed.
using Signature = std::array<std::byte, kSignatureSize>;  ///< An Ed25519 signature.

/// @brief Readies libsodium; idempotent and thread-safe. The cores call it when they are built.
/// @throws exceptions::CryptoException If libsodium cannot start.
AYLO_NETWORK_API void initialize();

/// @return 32 bytes of @p random: a seed for a key pair. @p random must be cryptographically secure outside tests
/// (makeSystemRandom() is).
[[nodiscard]] AYLO_NETWORK_API Key makeSeed(const RandomSource& random);

/// @brief The keys each side uses once the exchange is done: one to open what it receives, one to seal what it sends.
struct SessionKeys {
    Key receive{};   ///< Opens what the peer sends.
    Key transmit{};  ///< Seals what this side sends.
};

/// @brief An X25519 key pair for one key exchange (`crypto_kx`).
struct AYLO_NETWORK_API KeyExchange {
    Key publicKey{};  ///< Sent to the peer.
    Key secretKey{};  ///< Never leaves this side.

    /// @return The key pair derived from @p seed.
    [[nodiscard]] static KeyExchange fromSeed(const Key& seed);
};

/// @return The session keys of the client that holds @p client and received @p server, or nothing if @p server is not
/// a valid public key.
[[nodiscard]] AYLO_NETWORK_API std::optional<SessionKeys> clientSessionKeys(const KeyExchange& client,
                                                                            const Key& server);

/// @return The session keys of the server that holds @p server and received @p client, or nothing if @p client is not
/// a valid public key.
[[nodiscard]] AYLO_NETWORK_API std::optional<SessionKeys> serverSessionKeys(const KeyExchange& server,
                                                                            const Key& client);

/// @brief The long-lived Ed25519 key pair a server proves itself with: clients may pin its public key.
class AYLO_NETWORK_API Identity {
  public:
    /// @return The identity derived from @p seed: the same seed always gives the same identity.
    [[nodiscard]] static Identity fromSeed(const Key& seed);

    /// @return The public key, which clients may pin (`network.serverKey`).
    [[nodiscard]] const Key& publicKey() const noexcept;

    /// @return The signature of @p message.
    [[nodiscard]] Signature sign(std::span<const std::byte> message) const;

  private:
    Identity() = default;

    Key _publicKey{};                                 ///< Shown to clients.
    std::array<std::byte, kSignatureSize> _secret{};  ///< libsodium's secret key: the seed and the public key.
};

/// @return Whether @p signature is @p signer's signature of @p message.
[[nodiscard]] AYLO_NETWORK_API bool verify(const Key& signer, std::span<const std::byte> message,
                                           const Signature& signature) noexcept;

/// @return @p plaintext encrypted and authenticated with @p key (ChaCha20-Poly1305), followed by its tag: kTagSize
/// bytes more. @p additional is authenticated too, not encrypted.
/// @param counter The nonce: never twice with the same key and a different plaintext.
[[nodiscard]] AYLO_NETWORK_API Bytes seal(const Key& key, std::uint64_t counter, std::span<const std::byte> additional,
                                          std::span<const std::byte> plaintext);

/// @return The plaintext of @p sealed, or nothing if it was not sealed by @p key with @p counter and @p additional (a
/// forged, changed or truncated datagram).
[[nodiscard]] AYLO_NETWORK_API std::optional<Bytes> open(const Key& key, std::uint64_t counter,
                                                         std::span<const std::byte> additional,
                                                         std::span<const std::byte> sealed);

/// @brief Remembers which counters of the last 64 were received, so a datagram recorded and sent again is dropped.
class AYLO_NETWORK_API ReplayWindow {
  public:
    /// @return Whether @p counter was not received yet and is not older than the 64 most recent: check before opening.
    [[nodiscard]] bool isFresh(std::uint64_t counter) const noexcept;

    /// @brief Records @p counter as received: call once the datagram opened.
    void mark(std::uint64_t counter) noexcept;

  private:
    std::uint64_t _highest{0};  ///< The most recent counter received.
    std::uint64_t _seen{0};     ///< Bit N: `_highest - N` was received.
    bool _any{false};           ///< False until a counter is marked.
};

/// @return @p bytes in lowercase hexadecimal.
[[nodiscard]] AYLO_NETWORK_API std::string toHex(std::span<const std::byte> bytes);

/// @return The key written as 64 hexadecimal digits in @p text, or nothing if it is not one.
[[nodiscard]] AYLO_NETWORK_API std::optional<Key> keyFromHex(std::string_view text);
}  // namespace aylo::network::crypto
