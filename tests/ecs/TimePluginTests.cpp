/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TimePluginTests
*/

#include <gtest/gtest.h>

#include <chrono>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/Time.hpp"
#include "aylo/ecs/time/TimeClock.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"

namespace aylo::test::time_plugin {
struct FixedRuns {
    int count;
};
AYLO_ECS_RESOURCE(FixedRuns, "test.timePlugin.FixedRuns", count);
}  // namespace aylo::test::time_plugin

namespace {
using aylo::ecs::App;
using aylo::ecs::FixedTimestep;
using aylo::ecs::Phase;
using aylo::ecs::ResMut;
using aylo::ecs::Time;
using aylo::ecs::TimeClock;
using aylo::ecs::TimePlugin;
using aylo::test::time_plugin::FixedRuns;
using std::chrono::milliseconds;

void countFixed(ResMut<FixedRuns> runs) { ++runs->count; }

/// @return The App's clock as a TimeClock, or nullptr if it is another clock or none.
const TimeClock* timeClockOf(const App& app) { return dynamic_cast<const TimeClock*>(app.getSchedules().getClock()); }
}  // namespace

TEST(TimePlugin, IsNamedAyloTime) { EXPECT_EQ(TimePlugin{}.getName(), "aylo.time"); }

TEST(TimePlugin, MeasuresTheRealClockAtSixtyHertzByDefault) {
    const TimePlugin plugin;
    EXPECT_EQ(plugin.getFixedTimestep().getRate(), 60U);
    EXPECT_EQ(plugin.getFixedTimestep().getMaxTicksPerFrame(), 5U);
    EXPECT_FALSE(plugin.getManualFrame());
}

TEST(TimePlugin, CanUseManualFrames) {
    const TimePlugin plugin = TimePlugin::manual(milliseconds{16}, FixedTimestep{30, 2});
    EXPECT_EQ(plugin.getManualFrame(), milliseconds{16});
    EXPECT_EQ(plugin.getFixedTimestep().getRate(), 30U);
}

TEST(TimePlugin, InsertsTimeAndInstallsItsClock) {
    App app;
    app.addPlugin(TimePlugin::manual(milliseconds{20}, FixedTimestep{30, 5}));
    ASSERT_TRUE(app.getWorld().getResources().has<Time>());
    EXPECT_FLOAT_EQ(app.getWorld().getResources().require<Time>().fixedDelta, 1.F / 30.F);
    const TimeClock* clock = timeClockOf(app);
    ASSERT_NE(clock, nullptr);
    EXPECT_EQ(clock->getFixedTimestep().getRate(), 30U);
    EXPECT_EQ(clock->getManualFrame(), milliseconds{20});
}

TEST(TimePlugin, RunsFixedUpdateAtItsRate) {
    App app;
    app.insertResource(FixedRuns{.count = 0});
    app.addPlugin(TimePlugin::manual(milliseconds{50}));
    app.addSystem(Phase::kFixedUpdate, "test.countFixed", &countFixed);
    app.runFrame();
    app.runFrame();
    EXPECT_EQ(app.getWorld().getResources().require<FixedRuns>().count, 6);  // 3 ticks per 50 ms frame
    EXPECT_EQ(app.getWorld().getResources().require<Time>().tick, 6U);
}
