/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CryptoTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "aylo/network/Types.hpp"
#include "aylo/network/crypto/Crypto.hpp"
#include "aylo/network/protocol/Handshake.hpp"
#include "aylo/network/protocol/Packets.hpp"

namespace network = aylo::network;
namespace crypto = aylo::network::crypto;
namespace protocol = aylo::network::protocol;

namespace {
crypto::Key keyOf(const int value) {
    crypto::Key key{};
    key.fill(static_cast<std::byte>(value));
    return key;
}

network::Bytes bytesOf(const std::string& text) {
    network::Bytes bytes;
    for (const char character : text) {
        bytes.push_back(static_cast<std::byte>(character));
    }
    return bytes;
}

class CryptoTest : public ::testing::Test {
  protected:
    CryptoTest() { crypto::initialize(); }
};
}  // namespace

TEST_F(CryptoTest, ASealOpensWithItsKeyCounterAndAdditionalData) {
    const network::Bytes plaintext = bytesOf("snapshot");
    const network::Bytes additional = bytesOf("header");
    const network::Bytes sealed = crypto::seal(keyOf(1), 7, additional, plaintext);

    EXPECT_EQ(sealed.size(), plaintext.size() + crypto::kTagSize);
    EXPECT_NE(network::Bytes(sealed.begin(), sealed.begin() + 8), plaintext);  // not in clear
    EXPECT_EQ(crypto::open(keyOf(1), 7, additional, sealed), plaintext);
}

TEST_F(CryptoTest, ASealDoesNotOpenOnceAnythingChanged) {
    const network::Bytes additional = bytesOf("header");
    const network::Bytes sealed = crypto::seal(keyOf(1), 7, additional, bytesOf("snapshot"));

    EXPECT_FALSE(crypto::open(keyOf(2), 7, additional, sealed));
    EXPECT_FALSE(crypto::open(keyOf(1), 8, additional, sealed));
    EXPECT_FALSE(crypto::open(keyOf(1), 7, bytesOf("Header"), sealed));
    for (std::size_t index = 0; index < sealed.size(); ++index) {
        network::Bytes changed = sealed;
        changed[index] ^= std::byte{0x01};
        EXPECT_FALSE(crypto::open(keyOf(1), 7, additional, changed)) << index;
    }
    EXPECT_FALSE(crypto::open(keyOf(1), 7, additional, std::span{sealed}.first(crypto::kTagSize - 1)));
}

TEST_F(CryptoTest, BothEndsOfAKeyExchangeAgree) {
    const auto client = crypto::KeyExchange::fromSeed(keyOf(1));
    const auto server = crypto::KeyExchange::fromSeed(keyOf(2));
    const auto clientKeys = crypto::clientSessionKeys(client, server.publicKey);
    const auto serverKeys = crypto::serverSessionKeys(server, client.publicKey);

    ASSERT_TRUE(clientKeys);
    ASSERT_TRUE(serverKeys);
    EXPECT_EQ(clientKeys->transmit, serverKeys->receive);
    EXPECT_EQ(clientKeys->receive, serverKeys->transmit);
    EXPECT_NE(clientKeys->transmit, clientKeys->receive);
}

TEST_F(CryptoTest, AKeyExchangeRefusesAnInvalidPublicKey) {
    const auto client = crypto::KeyExchange::fromSeed(keyOf(1));
    EXPECT_FALSE(crypto::clientSessionKeys(client, crypto::Key{}));  // the all-zero point
}

TEST_F(CryptoTest, AnIdentityIsTheSameForTheSameSeedAndSignsWhatItSaid) {
    const auto identity = crypto::Identity::fromSeed(keyOf(3));
    const network::Bytes message = bytesOf("key exchange key");
    const crypto::Signature signature = identity.sign(message);

    EXPECT_EQ(identity.publicKey(), crypto::Identity::fromSeed(keyOf(3)).publicKey());
    EXPECT_NE(identity.publicKey(), crypto::Identity::fromSeed(keyOf(4)).publicKey());
    EXPECT_TRUE(crypto::verify(identity.publicKey(), message, signature));
    EXPECT_FALSE(crypto::verify(identity.publicKey(), bytesOf("another key"), signature));
    EXPECT_FALSE(crypto::verify(crypto::Identity::fromSeed(keyOf(4)).publicKey(), message, signature));
}

TEST_F(CryptoTest, ASeedTakesFourDrawsOfTheRandomSource) {
    std::uint64_t next = 0x0102030405060708ULL;
    const crypto::Key seed = crypto::makeSeed([&next] { return next++; });
    EXPECT_EQ(seed[0], std::byte{0x08});  // lowest byte first
    EXPECT_EQ(seed[8], std::byte{0x09});  // the next draw
    EXPECT_EQ(next, 0x0102030405060708ULL + 4);
}

TEST(ReplayWindow, AcceptsEachCounterOnce) {
    crypto::ReplayWindow window;
    EXPECT_TRUE(window.isFresh(5));
    window.mark(5);
    EXPECT_FALSE(window.isFresh(5));
    EXPECT_TRUE(window.isFresh(6));
}

TEST(ReplayWindow, AcceptsALateCounterInsideTheWindowOnce) {
    crypto::ReplayWindow window;
    window.mark(100);
    EXPECT_TRUE(window.isFresh(90));  // reordered by the network
    window.mark(90);
    EXPECT_FALSE(window.isFresh(90));
    EXPECT_TRUE(window.isFresh(37));
    EXPECT_FALSE(window.isFresh(36));  // 64 behind: too old to tell
}

TEST(ReplayWindow, ForgetsWhatSlidesOutOfTheWindow) {
    crypto::ReplayWindow window;
    window.mark(1);
    window.mark(200);
    EXPECT_FALSE(window.isFresh(1));
    EXPECT_FALSE(window.isFresh(200));
    EXPECT_TRUE(window.isFresh(199));
}

TEST(Hex, RoundTripsAKey) {
    crypto::Key key{};
    for (std::size_t index = 0; index < key.size(); ++index) {
        key.at(index) = static_cast<std::byte>(index * 7);
    }
    const std::string text = crypto::toHex(key);
    EXPECT_EQ(text.size(), 64U);
    EXPECT_EQ(text.substr(0, 4), "0007");
    EXPECT_EQ(crypto::keyFromHex(text), key);
    EXPECT_EQ(crypto::keyFromHex("ABCDEF" + std::string(58, '0'))->at(0), std::byte{0xAB});
}

TEST(Hex, RefusesWhatIsNotAKey) {
    EXPECT_FALSE(crypto::keyFromHex(""));
    EXPECT_FALSE(crypto::keyFromHex(std::string(63, '0')));
    EXPECT_FALSE(crypto::keyFromHex(std::string(65, '0')));
    EXPECT_FALSE(crypto::keyFromHex(std::string(63, '0') + "g"));
}

TEST_F(CryptoTest, AcceptedOpensOnlyWithTheClientsKey) {
    const protocol::Accepted accepted{
        .sealed = protocol::sealAccepted(keyOf(1), {.sessionToken = 3, .client = 4, .resumeToken = 5})};

    const auto body = protocol::openAccepted(keyOf(1), accepted);
    ASSERT_TRUE(body);
    EXPECT_EQ(body->sessionToken, 3U);
    EXPECT_EQ(body->client, 4U);
    EXPECT_EQ(body->resumeToken, 5U);
    EXPECT_FALSE(protocol::openAccepted(keyOf(2), accepted));
}

TEST_F(CryptoTest, AResumeRequestIsBoundToItsChallenge) {
    const protocol::ChallengeResponse response{
        .challenge = 42, .sealedResume = protocol::sealResume(keyOf(1), 42, {.client = 6, .token = 7})};

    const auto request = protocol::openResume(keyOf(1), response);
    ASSERT_TRUE(request);
    EXPECT_EQ(request->client, 6U);
    EXPECT_EQ(request->token, 7U);
    protocol::ChallengeResponse moved = response;
    moved.challenge = 43;  // replayed for another challenge
    EXPECT_FALSE(protocol::openResume(keyOf(1), moved));
}
