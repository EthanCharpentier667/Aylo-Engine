/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TestKeys
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "aylo/network/Types.hpp"
#include "aylo/network/bytes/ByteReader.hpp"
#include "aylo/network/bytes/ByteWriter.hpp"
#include "aylo/network/crypto/Crypto.hpp"
#include "aylo/network/protocol/Constants.hpp"
#include "aylo/network/protocol/Packets.hpp"

namespace aylo::network::tests {
/// @brief The session keys both ends of a test connection hold, as a key exchange gives them: what the client
/// transmits, the server receives, and the other way round.
struct TestKeys {
    crypto::SessionKeys client;  ///< The client's end.
    crypto::SessionKeys server;  ///< The server's end.
};

/// @return Fixed keys for tests, derived once: a key exchange costs far more than the datagrams a test opens.
inline TestKeys makeTestKeys() {
    crypto::initialize();
    crypto::Key clientSeed{};
    crypto::Key serverSeed{};
    clientSeed.fill(std::byte{1});
    serverSeed.fill(std::byte{2});
    const auto client = crypto::KeyExchange::fromSeed(clientSeed);
    const auto server = crypto::KeyExchange::fromSeed(serverSeed);
    return TestKeys{.client = *crypto::clientSessionKeys(client, server.publicKey),
                    .server = *crypto::serverSessionKeys(server, client.publicKey)};
}

/// @return The keys of makeTestKeys(), the same every call.
inline const TestKeys& testKeys() {
    static const TestKeys kKeys = makeTestKeys();
    return kKeys;
}

/// @return The client's end of testKeys().
inline crypto::SessionKeys clientKeys() { return testKeys().client; }

/// @return The server's end of testKeys().
inline crypto::SessionKeys serverKeys() { return testKeys().server; }

/// @brief What a `Data` datagram holds, opened.
struct OpenedData {
    std::uint32_t sessionToken{0};  ///< In clear.
    std::uint64_t counter{0};       ///< In clear.
    std::uint16_t sequence{0};      ///< Sealed.
    std::uint16_t ack{0};           ///< Sealed.
    std::uint32_t ackBits{0};       ///< Sealed.
    Bytes messages;                 ///< Sealed.
};

/// @return The clear header of a datagram: the additional data of its seal.
inline std::array<std::byte, 13> clearHeader(const protocol::PacketKind kind, const std::uint32_t token,
                                             const std::uint64_t counter) {
    std::array<std::byte, 13> header{};
    bytes::ByteWriter writer{header};
    writer.writeU8(static_cast<std::uint8_t>(kind));
    writer.writeU32(token);
    writer.writeU64(counter);
    return header;
}

/// @return @p datagram opened, if it is a `Data` sealed by either end of testKeys().
inline std::optional<OpenedData> openData(const std::span<const std::byte> datagram) {
    const auto decoded = protocol::PacketCodec::decode(datagram);
    if (!decoded || !std::holds_alternative<protocol::Data>(*decoded)) {
        return std::nullopt;
    }
    const auto& data = std::get<protocol::Data>(*decoded);
    const auto header = clearHeader(protocol::PacketKind::kData, data.sessionToken, data.counter);
    const TestKeys& keys = testKeys();
    auto body = crypto::open(keys.client.transmit, data.counter, header, data.sealed);
    if (!body) {
        body = crypto::open(keys.server.transmit, data.counter, header, data.sealed);
    }
    if (!body) {
        return std::nullopt;
    }
    bytes::ByteReader reader{*body};
    OpenedData opened{.sessionToken = data.sessionToken, .counter = data.counter};
    opened.sequence = reader.readU16();
    opened.ack = reader.readU16();
    opened.ackBits = reader.readU32();
    const auto messages = reader.readBytes(reader.remainingSize());
    opened.messages.assign(messages.begin(), messages.end());
    return opened;
}

/// @return A `Data` datagram with these fields, sealed with @p transmit.
inline Bytes sealData(const crypto::Key& transmit, const std::uint32_t token, const std::uint64_t counter,
                      const std::uint16_t sequence, const std::uint16_t ack, const std::uint32_t ackBits,
                      const std::span<const std::byte> messages) {
    Bytes body(8 + messages.size());
    bytes::ByteWriter writer{body};
    writer.writeU16(sequence);
    writer.writeU16(ack);
    writer.writeU32(ackBits);
    writer.writeBytes(messages);
    const Bytes sealed =
        crypto::seal(transmit, counter, clearHeader(protocol::PacketKind::kData, token, counter), body);
    return protocol::PacketCodec::encode(protocol::Data{.sessionToken = token, .counter = counter, .sealed = sealed});
}
}  // namespace aylo::network::tests
