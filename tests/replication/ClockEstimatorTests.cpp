/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ClockEstimatorTests
*/

#include <gtest/gtest.h>

#include <limits>

#include "aylo/replication/timesync/ClockEstimator.hpp"

using aylo::replication::timesync::ClockEstimator;

TEST(ClockEstimator, KnowsNothingBeforeTheFirstExchange) {
    const ClockEstimator clock;
    EXPECT_FALSE(clock.isSynced());
    EXPECT_FALSE(clock.serverTime(10.0));
    EXPECT_EQ(clock.rtt(), 0.0);
}

TEST(ClockEstimator, FindsTheOffsetOfASymmetricLink) {
    ClockEstimator clock;
    // The server clock is 100 s ahead; 20 ms each way.
    ASSERT_TRUE(clock.addSample(1.0, 1.04, 101.02));
    ASSERT_TRUE(clock.isSynced());
    EXPECT_NEAR(*clock.serverTime(2.0), 102.0, 1e-9);
    EXPECT_NEAR(clock.rtt(), 0.04, 1e-9);
}

TEST(ClockEstimator, TrustsTheExchangeWithTheSmallestRoundTrip) {
    ClockEstimator clock;
    ASSERT_TRUE(clock.addSample(1.0, 1.02, 51.01));  // good: 20 ms, offset 50
    ASSERT_TRUE(clock.addSample(2.0, 2.30, 52.28));  // a spike on the way back: 300 ms, offset 50.13
    EXPECT_NEAR(*clock.serverTime(3.0), 53.0, 1e-9);
}

TEST(ClockEstimator, ForgetsTheExchangesThatLeftTheWindow) {
    ClockEstimator clock;
    ASSERT_TRUE(clock.addSample(1.0, 1.001, 11.0005));  // the best, offset 10
    for (std::size_t i = 0; i < ClockEstimator::kWindow; ++i) {
        const double sent = 10.0 + static_cast<double>(i);
        ASSERT_TRUE(clock.addSample(sent, sent + 0.02, sent + 20.01));  // offset 20, each 20 ms
    }
    EXPECT_NEAR(*clock.serverTime(0.0), 20.0, 1e-9);
}

TEST(ClockEstimator, IgnoresAnExchangeThatMakesNoSense) {
    ClockEstimator clock;
    EXPECT_FALSE(clock.addSample(2.0, 1.0, 5.0));  // the pong before the ping
    EXPECT_FALSE(clock.addSample(0.0, ClockEstimator::kMaxRoundTrip + 1.0, 5.0));
    EXPECT_FALSE(clock.addSample(0.0, 1.0, std::numeric_limits<double>::quiet_NaN()));
    EXPECT_FALSE(clock.isSynced());
}

TEST(ClockEstimator, StartsAgainAfterAReset) {
    ClockEstimator clock;
    ASSERT_TRUE(clock.addSample(1.0, 1.02, 51.01));
    clock.reset();
    EXPECT_FALSE(clock.isSynced());
    EXPECT_FALSE(clock.serverTime(1.0));
    ASSERT_TRUE(clock.addSample(1.0, 1.02, 201.01));
    EXPECT_NEAR(*clock.serverTime(2.0), 202.0, 1e-9);
}
