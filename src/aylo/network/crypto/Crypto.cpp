/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Crypto
*/

#include "aylo/network/crypto/Crypto.hpp"

#include <sodium/core.h>
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sodium/crypto_kx.h>
#include <sodium/crypto_sign.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "aylo/network/RandomSource.hpp"
#include "aylo/network/Types.hpp"
#include "aylo/network/exceptions/NetworkExceptions.hpp"

namespace aylo::network::crypto {
namespace {
static_assert(kKeySize == crypto_kx_PUBLICKEYBYTES);
static_assert(kKeySize == crypto_kx_SECRETKEYBYTES);
static_assert(kKeySize == crypto_kx_SESSIONKEYBYTES);
static_assert(kKeySize == crypto_kx_SEEDBYTES);
static_assert(kKeySize == crypto_sign_PUBLICKEYBYTES);
static_assert(kKeySize == crypto_sign_SEEDBYTES);
static_assert(kSignatureSize == crypto_sign_BYTES);
static_assert(kSignatureSize == crypto_sign_SECRETKEYBYTES);
static_assert(kKeySize == crypto_aead_chacha20poly1305_ietf_KEYBYTES);
static_assert(kTagSize == crypto_aead_chacha20poly1305_ietf_ABYTES);

/// @brief Bits of the replay window: how far behind the most recent counter a datagram may still arrive.
constexpr std::uint64_t kWindow = 64;

/// @return libsodium's view of bytes it writes.
auto* writable(std::span<std::byte> bytes) noexcept {
    return reinterpret_cast<unsigned char*>(bytes.data());  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

/// @return libsodium's view of bytes it reads.
const auto* readable(std::span<const std::byte> bytes) noexcept {
    return reinterpret_cast<const unsigned char*>(bytes.data());  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

/// @return The 12-byte nonce of @p counter: four zero bytes, then the counter, lowest byte first.
std::array<unsigned char, crypto_aead_chacha20poly1305_ietf_NPUBBYTES> nonceOf(std::uint64_t counter) {
    std::array<unsigned char, crypto_aead_chacha20poly1305_ietf_NPUBBYTES> nonce{};
    for (std::size_t index = 4; index < nonce.size(); ++index) {
        nonce.at(index) = static_cast<unsigned char>(counter & 0xFFU);
        counter >>= 8U;
    }
    return nonce;
}

/// @return The value of a hexadecimal digit, or nothing.
std::optional<std::uint8_t> digit(const char character) noexcept {
    if (character >= '0' && character <= '9') {
        return static_cast<std::uint8_t>(character - '0');
    }
    if (character >= 'a' && character <= 'f') {
        return static_cast<std::uint8_t>(character - 'a' + 10);
    }
    if (character >= 'A' && character <= 'F') {
        return static_cast<std::uint8_t>(character - 'A' + 10);
    }
    return std::nullopt;
}
}  // namespace

void initialize() {
    if (sodium_init() < 0) {
        throw exceptions::CryptoException{"libsodium could not start"};
    }
}

Key makeSeed(const RandomSource& random) {
    Key seed{};
    for (std::size_t offset = 0; offset < seed.size(); offset += sizeof(std::uint64_t)) {
        std::uint64_t value = random();
        for (std::size_t index = 0; index < sizeof(std::uint64_t); ++index) {
            seed.at(offset + index) = static_cast<std::byte>(value & 0xFFU);
            value >>= 8U;
        }
    }
    return seed;
}

KeyExchange KeyExchange::fromSeed(const Key& seed) {
    KeyExchange pair;
    crypto_kx_seed_keypair(writable(pair.publicKey), writable(pair.secretKey), readable(seed));
    return pair;
}

std::optional<SessionKeys> clientSessionKeys(const KeyExchange& client, const Key& server) {
    SessionKeys keys;
    if (crypto_kx_client_session_keys(writable(keys.receive), writable(keys.transmit), readable(client.publicKey),
                                      readable(client.secretKey), readable(server)) != 0) {
        return std::nullopt;
    }
    return keys;
}

std::optional<SessionKeys> serverSessionKeys(const KeyExchange& server, const Key& client) {
    SessionKeys keys;
    if (crypto_kx_server_session_keys(writable(keys.receive), writable(keys.transmit), readable(server.publicKey),
                                      readable(server.secretKey), readable(client)) != 0) {
        return std::nullopt;
    }
    return keys;
}

Identity Identity::fromSeed(const Key& seed) {
    Identity identity;
    crypto_sign_seed_keypair(writable(identity._publicKey), writable(identity._secret), readable(seed));
    return identity;
}

const Key& Identity::publicKey() const noexcept { return _publicKey; }

Signature Identity::sign(const std::span<const std::byte> message) const {
    Signature signature{};
    crypto_sign_detached(writable(signature), nullptr, readable(message), message.size(), readable(_secret));
    return signature;
}

bool verify(const Key& signer, const std::span<const std::byte> message, const Signature& signature) noexcept {
    return crypto_sign_verify_detached(readable(signature), readable(message), message.size(), readable(signer)) == 0;
}

Bytes seal(const Key& key, const std::uint64_t counter, const std::span<const std::byte> additional,
           const std::span<const std::byte> plaintext) {
    Bytes sealed(plaintext.size() + kTagSize);
    unsigned long long length = 0;  // NOLINT(google-runtime-int): libsodium's type
    const auto nonce = nonceOf(counter);
    crypto_aead_chacha20poly1305_ietf_encrypt(writable(sealed), &length, readable(plaintext), plaintext.size(),
                                              readable(additional), additional.size(), nullptr, nonce.data(),
                                              readable(key));
    sealed.resize(static_cast<std::size_t>(length));
    return sealed;
}

std::optional<Bytes> open(const Key& key, const std::uint64_t counter, const std::span<const std::byte> additional,
                          const std::span<const std::byte> sealed) {
    if (sealed.size() < kTagSize) {
        return std::nullopt;
    }
    Bytes plaintext(sealed.size() - kTagSize);
    unsigned long long length = 0;  // NOLINT(google-runtime-int): libsodium's type
    const auto nonce = nonceOf(counter);
    if (crypto_aead_chacha20poly1305_ietf_decrypt(writable(plaintext), &length, nullptr, readable(sealed),
                                                  sealed.size(), readable(additional), additional.size(), nonce.data(),
                                                  readable(key)) != 0) {
        return std::nullopt;
    }
    plaintext.resize(static_cast<std::size_t>(length));
    return plaintext;
}

bool ReplayWindow::isFresh(const std::uint64_t counter) const noexcept {
    if (!_any || counter > _highest) {
        return true;
    }
    const std::uint64_t behind = _highest - counter;
    return behind < kWindow && ((_seen >> behind) & 1U) == 0;
}

void ReplayWindow::mark(const std::uint64_t counter) noexcept {
    if (!_any) {
        _any = true;
        _highest = counter;
        _seen = 1;
        return;
    }
    if (counter > _highest) {
        const std::uint64_t ahead = counter - _highest;
        _seen = ahead >= kWindow ? 0 : _seen << ahead;
        _seen |= 1U;
        _highest = counter;
        return;
    }
    if (const std::uint64_t behind = _highest - counter; behind < kWindow) {
        _seen |= std::uint64_t{1} << behind;
    }
}

std::string toHex(const std::span<const std::byte> bytes) {
    constexpr std::string_view kDigits = "0123456789abcdef";
    std::string text;
    text.reserve(bytes.size() * 2);
    for (const std::byte byte : bytes) {
        const auto value = std::to_integer<std::uint8_t>(byte);
        text.push_back(kDigits.at(value >> 4U));
        text.push_back(kDigits.at(value & 0x0FU));
    }
    return text;
}

std::optional<Key> keyFromHex(const std::string_view text) {
    Key key{};
    if (text.size() != key.size() * 2) {
        return std::nullopt;
    }
    for (std::size_t index = 0; index < key.size(); ++index) {
        const auto high = digit(text.at(index * 2));
        const auto low = digit(text.at((index * 2) + 1));
        if (!high || !low) {
            return std::nullopt;
        }
        key.at(index) = static_cast<std::byte>((*high << 4U) | *low);
    }
    return key;
}
}  // namespace aylo::network::crypto
