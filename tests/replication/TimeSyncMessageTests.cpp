/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TimeSyncMessageTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <limits>

#include "aylo/network/Types.hpp"
#include "aylo/replication/MessageType.hpp"
#include "aylo/replication/timesync/TimeSyncMessage.hpp"

using aylo::replication::timesync::decodePing;
using aylo::replication::timesync::decodePong;
using aylo::replication::timesync::encodePing;
using aylo::replication::timesync::encodePong;

TEST(TimeSyncMessage, RoundTripsAPing) {
    const aylo::network::Bytes bytes = encodePing({.clientTime = 3.25});
    EXPECT_EQ(bytes.front(), static_cast<std::byte>(aylo::replication::MessageType::kTimePing));
    const auto ping = decodePing(bytes);
    ASSERT_TRUE(ping);
    EXPECT_DOUBLE_EQ(ping->clientTime, 3.25);
}

TEST(TimeSyncMessage, RoundTripsAPong) {
    const aylo::network::Bytes bytes = encodePong({.clientTime = 3.25, .serverTime = 100.5});
    EXPECT_EQ(bytes.front(), static_cast<std::byte>(aylo::replication::MessageType::kTimePong));
    const auto pong = decodePong(bytes);
    ASSERT_TRUE(pong);
    EXPECT_DOUBLE_EQ(pong->clientTime, 3.25);
    EXPECT_DOUBLE_EQ(pong->serverTime, 100.5);
}

TEST(TimeSyncMessage, DropsWhatIsNotAWellFormedMessageWithoutThrowing) {
    aylo::network::Bytes ping = encodePing({.clientTime = 1.0});
    aylo::network::Bytes pong = encodePong({.clientTime = 1.0, .serverTime = 2.0});
    EXPECT_FALSE(decodePing(aylo::network::Bytes{}));
    EXPECT_FALSE(decodePong(aylo::network::Bytes{}));
    EXPECT_FALSE(decodePing(pong));  // another type
    EXPECT_FALSE(decodePong(ping));
    for (std::size_t size = 0; size < ping.size(); ++size) {
        EXPECT_FALSE(decodePing(aylo::network::Bytes(ping.begin(), ping.begin() + static_cast<std::ptrdiff_t>(size))));
    }
    for (std::size_t size = 0; size < pong.size(); ++size) {
        EXPECT_FALSE(decodePong(aylo::network::Bytes(pong.begin(), pong.begin() + static_cast<std::ptrdiff_t>(size))));
    }
    ping.push_back(std::byte{0});
    pong.push_back(std::byte{0});
    EXPECT_FALSE(decodePing(ping));  // bytes left over
    EXPECT_FALSE(decodePong(pong));
}

TEST(TimeSyncMessage, RefusesATimeThatIsNotAFiniteNumberOfSeconds) {
    for (const double bad : {-1.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
        EXPECT_FALSE(decodePing(encodePing({.clientTime = bad})));
        EXPECT_FALSE(decodePong(encodePong({.clientTime = bad, .serverTime = 1.0})));
        EXPECT_FALSE(decodePong(encodePong({.clientTime = 1.0, .serverTime = bad})));
    }
}
