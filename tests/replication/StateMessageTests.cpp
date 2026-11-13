/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StateMessageTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>

#include "aylo/network/Types.hpp"
#include "aylo/replication/MessageType.hpp"
#include "aylo/replication/state/StateMessage.hpp"

using aylo::replication::state::decodeState;
using aylo::replication::state::encodeState;

TEST(StateMessage, RoundTripsTheMachineAndItsValue) {
    const std::optional<aylo::network::Bytes> bytes = encodeState("game.Phase", "Playing");
    ASSERT_TRUE(bytes);
    EXPECT_EQ(bytes->front(), static_cast<std::byte>(aylo::replication::MessageType::kState));
    const auto decoded = decodeState(*bytes);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->machine, "game.Phase");
    EXPECT_EQ(decoded->value, "Playing");
}

TEST(StateMessage, RefusesWhatDoesNotFitOnTheWire) {
    EXPECT_FALSE(encodeState("", "Playing"));
    EXPECT_FALSE(encodeState("game.Phase", ""));
    EXPECT_FALSE(encodeState("game.Phase", std::string(256, 'a')));
    EXPECT_TRUE(encodeState("game.Phase", std::string(255, 'a')));
}

TEST(StateMessage, DropsABadMessageWithoutThrowing) {
    aylo::network::Bytes bytes = *encodeState("game.Phase", "Playing");
    EXPECT_FALSE(decodeState(aylo::network::Bytes{}));
    aylo::network::Bytes truncated{bytes.begin(), bytes.end() - 1};
    EXPECT_FALSE(decodeState(truncated));
    aylo::network::Bytes longer = bytes;
    longer.push_back(std::byte{0});
    EXPECT_FALSE(decodeState(longer));
    aylo::network::Bytes otherType = bytes;
    otherType.front() = static_cast<std::byte>(aylo::replication::MessageType::kSnapshot);
    EXPECT_FALSE(decodeState(otherType));
    const aylo::network::Bytes emptyName{static_cast<std::byte>(0x33), std::byte{0}, std::byte{1}, std::byte{'A'}};
    EXPECT_FALSE(decodeState(emptyName));
}
