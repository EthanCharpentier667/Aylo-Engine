/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PacketsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

#include "aylo/network/Types.hpp"
#include "aylo/network/bytes/Overloaded.hpp"
#include "aylo/network/protocol/Constants.hpp"
#include "aylo/network/protocol/Packets.hpp"

namespace network = aylo::network;
namespace protocol = aylo::network::protocol;

namespace {
std::vector<std::byte> toBytes(const std::vector<int>& values) {
    std::vector<std::byte> result;
    for (const int value : values) {
        result.push_back(static_cast<std::byte>(value));
    }
    return result;
}

template <typename P>
std::optional<P> roundTrip(const P& packet) {
    const network::Bytes encoded = protocol::PacketCodec::encode(packet);
    const std::optional<protocol::Packet> decoded = protocol::PacketCodec::decode(encoded);
    if (!decoded || !std::holds_alternative<P>(*decoded)) {
        return std::nullopt;
    }
    return std::get<P>(*decoded);
}

/// @brief A valid 160-byte connect request, to be altered by the tests.
network::Bytes validConnectRequest() { return protocol::PacketCodec::encode(protocol::ConnectRequest{.version = 2}); }

/// @return A key whose bytes are all @p value.
aylo::network::crypto::Key keyOf(const int value) {
    aylo::network::crypto::Key key{};
    key.fill(static_cast<std::byte>(value));
    return key;
}
}  // namespace

TEST(Packets, ConnectRequestRoundTripsAndIs160Bytes) {
    const protocol::ConnectRequest packet{.version = 3};

    EXPECT_EQ(protocol::PacketCodec::encode(packet).size(), 160U);
    const auto decoded = roundTrip(packet);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->version, 3);
}

TEST(Packets, ConnectRequestLayout) {
    const network::Bytes encoded = protocol::PacketCodec::encode(protocol::ConnectRequest{.version = 0x0102});

    ASSERT_EQ(encoded.size(), 160U);
    const std::vector<std::byte> head = toBytes({0x01, 0x4F, 0x4C, 0x59, 0x41, 0x02, 0x01});
    EXPECT_TRUE(std::equal(head.begin(), head.end(), encoded.begin()));
    for (std::size_t index = head.size(); index < encoded.size(); ++index) {
        EXPECT_EQ(encoded[index], std::byte{0}) << index;
    }
}

TEST(Packets, ChallengeRoundTripsWithTheServerKeysAndIs137Bytes) {
    protocol::Challenge packet{.challenge = 0xAABBCCDD11223344ULL, .identity = keyOf(1), .exchange = keyOf(2)};
    packet.signature.fill(std::byte{3});

    EXPECT_EQ(protocol::PacketCodec::encode(packet).size(), 137U);
    const auto decoded = roundTrip(packet);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->challenge, packet.challenge);
    EXPECT_EQ(decoded->identity, keyOf(1));
    EXPECT_EQ(decoded->exchange, keyOf(2));
    EXPECT_EQ(decoded->signature, packet.signature);
}

TEST(Packets, ChallengeResponseRoundTripsWithTheClientKeyAndIs160Bytes) {
    protocol::ChallengeResponse packet{.challenge = 99, .exchange = keyOf(4)};
    packet.sealedResume.fill(std::byte{5});

    EXPECT_EQ(protocol::PacketCodec::encode(packet).size(), 160U);
    const auto decoded = roundTrip(packet);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->challenge, 99U);
    EXPECT_EQ(decoded->exchange, keyOf(4));
    EXPECT_EQ(decoded->sealedResume, packet.sealedResume);
}

TEST(Packets, AcceptedCarriesOnlyItsSealAndIs33Bytes) {
    protocol::Accepted packet;
    packet.sealed.fill(std::byte{6});

    EXPECT_EQ(protocol::PacketCodec::encode(packet).size(), 33U);
    const auto decoded = roundTrip(packet);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->sealed, packet.sealed);
}

TEST(Packets, RejectedRoundTripsAndIs2Bytes) {
    const protocol::Rejected packet{.reason = network::RejectReason::kServerFull};

    EXPECT_EQ(protocol::PacketCodec::encode(packet), toBytes({0x05, 2}));
    const auto decoded = roundTrip(packet);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->reason, network::RejectReason::kServerFull);
}

TEST(Packets, RejectedRefusesAReasonOutOfRangeAndZero) {
    EXPECT_FALSE(protocol::PacketCodec::decode(toBytes({0x05, 0})));
    EXPECT_FALSE(protocol::PacketCodec::decode(toBytes({0x05, 4})));
    EXPECT_TRUE(protocol::PacketCodec::decode(toBytes({0x05, 3})));
}

TEST(Packets, DataKeepsTheTokenAndCounterInClearAndPointsIntoTheDatagram) {
    const std::vector<std::byte> sealed = toBytes({1, 2, 3, 4, 5});
    const network::Bytes encoded = protocol::PacketCodec::encode(
        protocol::Data{.sessionToken = 0x01020304, .counter = 0x05060708090A0B0CULL, .sealed = sealed});

    EXPECT_EQ(encoded.size(), 13U + sealed.size());
    EXPECT_EQ(std::vector<std::byte>(encoded.begin(), encoded.begin() + 13),
              toBytes({0x06, 4, 3, 2, 1, 0x0C, 0x0B, 0x0A, 9, 8, 7, 6, 5}));
    const std::optional<protocol::Packet> decoded = protocol::PacketCodec::decode(encoded);
    ASSERT_TRUE(decoded);
    const auto& data = std::get<protocol::Data>(*decoded);
    EXPECT_EQ(data.sessionToken, 0x01020304U);
    EXPECT_EQ(data.counter, 0x05060708090A0B0CULL);
    ASSERT_EQ(data.sealed.size(), sealed.size());
    EXPECT_EQ(data.sealed.data(), encoded.data() + 13);
}

TEST(Packets, DataTruncatedInItsHeaderIsRejected) {
    const network::Bytes encoded = protocol::PacketCodec::encode(protocol::Data{});

    for (std::size_t size = 0; size < encoded.size(); ++size) {
        EXPECT_FALSE(protocol::PacketCodec::decode(std::span{encoded}.first(size))) << size;
    }
}

TEST(Packets, DisconnectRoundTripsAndIs30Bytes) {
    protocol::Disconnect packet{.sessionToken = 0xCAFEBABE, .counter = 7};
    packet.sealed.fill(std::byte{8});
    const network::Bytes encoded = protocol::PacketCodec::encode(packet);

    EXPECT_EQ(encoded.size(), 30U);
    EXPECT_EQ(std::vector<std::byte>(encoded.begin(), encoded.begin() + 5), toBytes({0x07, 0xBE, 0xBA, 0xFE, 0xCA}));
    const auto decoded = roundTrip(packet);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->sessionToken, 0xCAFEBABEU);
    EXPECT_EQ(decoded->counter, 7U);
    EXPECT_EQ(decoded->sealed, packet.sealed);
}

TEST(Packets, ConnectRequestWithAWrongProtocolIdIsRejected) {
    for (std::size_t index = 1; index <= 4; ++index) {
        network::Bytes encoded = validConnectRequest();
        encoded[index] ^= std::byte{0x01};
        EXPECT_FALSE(protocol::PacketCodec::decode(encoded)) << index;
    }
    EXPECT_TRUE(protocol::PacketCodec::decode(validConnectRequest()));
}

TEST(Packets, PaddedPacketsOneByteShortOrLongAreRejected) {
    network::Bytes connect = validConnectRequest();
    connect.pop_back();
    EXPECT_FALSE(protocol::PacketCodec::decode(connect));
    connect.push_back(std::byte{0});
    connect.push_back(std::byte{0});
    EXPECT_FALSE(protocol::PacketCodec::decode(connect));

    network::Bytes response = protocol::PacketCodec::encode(protocol::ChallengeResponse{.challenge = 1});
    ASSERT_EQ(response.size(), 160U);
    response.pop_back();
    EXPECT_FALSE(protocol::PacketCodec::decode(response));
    response.push_back(std::byte{0});
    response.push_back(std::byte{0});
    EXPECT_FALSE(protocol::PacketCodec::decode(response));
}

TEST(Packets, FixedSizePacketsRejectBytesLeftOver) {
    network::Bytes challenge = protocol::PacketCodec::encode(protocol::Challenge{.challenge = 1});
    challenge.push_back(std::byte{0});
    EXPECT_FALSE(protocol::PacketCodec::decode(challenge));

    network::Bytes accepted = protocol::PacketCodec::encode(protocol::Accepted{});
    accepted.push_back(std::byte{0});
    EXPECT_FALSE(protocol::PacketCodec::decode(accepted));
}

TEST(Packets, KindZeroAndKindEightAreUnknown) {
    EXPECT_FALSE(protocol::PacketCodec::decode(toBytes({0x00})));
    EXPECT_FALSE(protocol::PacketCodec::decode(toBytes({0x08, 1, 2, 3})));
    EXPECT_FALSE(protocol::PacketCodec::peekKind(toBytes({0x00})));
    EXPECT_EQ(protocol::PacketCodec::peekKind(toBytes({0x04})), protocol::PacketKind::kAccepted);
}

TEST(Packets, DecodeVisitsTheMatchingAlternative) {
    const network::Bytes encoded = protocol::PacketCodec::encode(protocol::Challenge{.challenge = 5});
    const std::optional<protocol::Packet> decoded = protocol::PacketCodec::decode(encoded);

    ASSERT_TRUE(decoded);
    const std::uint64_t seen =
        std::visit(network::bytes::Overloaded{[](const protocol::Challenge& challenge) { return challenge.challenge; },
                                              [](const auto&) { return std::uint64_t{0}; }},
                   *decoded);
    EXPECT_EQ(seen, 5U);
}

TEST(Packets, EveryServerReplyIsSmallerThanTheHandshakeRequests) {
    // Anti-amplification: no answer to a 160-byte request is bigger than the request.
    for (const std::size_t size : {protocol::PacketCodec::encode(protocol::Challenge{}).size(),
                                   protocol::PacketCodec::encode(protocol::Accepted{}).size(),
                                   protocol::PacketCodec::encode(protocol::Rejected{}).size()}) {
        EXPECT_LT(size, protocol::kHandshakePadding);
    }
    EXPECT_EQ(protocol::PacketCodec::encode(protocol::ConnectRequest{}).size(), protocol::kHandshakePadding);
    EXPECT_EQ(protocol::PacketCodec::encode(protocol::ChallengeResponse{}).size(), protocol::kHandshakePadding);
}

TEST(WireValues, ZeroIsNeverValid) {
    EXPECT_FALSE(isValidWireValue(static_cast<protocol::PacketKind>(0)));
    EXPECT_FALSE(isValidWireValue(static_cast<network::RejectReason>(0)));
    EXPECT_FALSE(isValidWireValue(static_cast<network::DisconnectReason>(0)));
    EXPECT_TRUE(isValidWireValue(network::Channel::kUnreliable));
    EXPECT_TRUE(isValidWireValue(network::Channel::kReliable));
    EXPECT_FALSE(isValidWireValue(static_cast<network::Channel>(2)));
}
