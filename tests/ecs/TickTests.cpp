/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TickTests
*/

#include <gtest/gtest.h>

#include <limits>

#include "aylo/ecs/change/Tick.hpp"

namespace {
constexpr aylo::ecs::Tick kMaxTick = std::numeric_limits<aylo::ecs::Tick>::max();
}  // namespace

static_assert(aylo::ecs::isNewerThan(5, 3, 10), "usable at compile time");

TEST(Tick, ATickAfterTheLastRunIsNewer) {
    EXPECT_TRUE(aylo::ecs::isNewerThan(8, 5, 10));
    EXPECT_FALSE(aylo::ecs::isNewerThan(3, 5, 10));
}

TEST(Tick, ATickAtTheLastRunIsNotNewer) { EXPECT_FALSE(aylo::ecs::isNewerThan(5, 5, 10)); }

TEST(Tick, ATickAtThisRunIsNewerThanAnEarlierLastRun) { EXPECT_TRUE(aylo::ecs::isNewerThan(10, 9, 10)); }

TEST(Tick, ComparesAcrossTheWrap) {
    // The counter wrapped between the last run and now: thisRun is small, lastRun near the maximum.
    const aylo::ecs::Tick lastRun = kMaxTick - 5;
    const aylo::ecs::Tick thisRun = 10;
    EXPECT_TRUE(aylo::ecs::isNewerThan(3, lastRun, thisRun));              // after the wrap
    EXPECT_TRUE(aylo::ecs::isNewerThan(kMaxTick - 1, lastRun, thisRun));   // before the wrap, after lastRun
    EXPECT_FALSE(aylo::ecs::isNewerThan(kMaxTick - 9, lastRun, thisRun));  // before lastRun
}

TEST(Tick, ClampLeavesRecentTicksAlone) {
    aylo::ecs::Tick tick = 1000;
    aylo::ecs::clampTick(tick, 5000);
    EXPECT_EQ(tick, 1000U);
}

TEST(Tick, ClampMovesTooOldTicksToTheMaximumAge) {
    const aylo::ecs::Tick thisRun = 4'000'000'000;
    aylo::ecs::Tick tick = 1;  // older than kMaxChangeAge seen from thisRun
    aylo::ecs::clampTick(tick, thisRun);
    EXPECT_EQ(thisRun - tick, aylo::ecs::kMaxChangeAge);
}

TEST(Tick, AClampedTickStaysOlderThanRecentOnes) {
    const aylo::ecs::Tick thisRun = 4'000'000'000;
    aylo::ecs::Tick old = 1;
    aylo::ecs::clampTick(old, thisRun);
    const aylo::ecs::Tick lastRun = thisRun - 100;
    EXPECT_FALSE(aylo::ecs::isNewerThan(old, lastRun, thisRun));
    EXPECT_TRUE(aylo::ecs::isNewerThan(thisRun - 50, lastRun, thisRun));
}
