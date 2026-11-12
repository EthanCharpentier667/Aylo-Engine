/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ResourceParamTests
*/

#include <gtest/gtest.h>

#include <string_view>
#include <type_traits>
#include <utility>

#include "aylo/ecs/change/Tick.hpp"
#include "aylo/ecs/exceptions/MissingResourceException.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::test::resource_param {
struct Time {
    float delta;
};
AYLO_ECS_RESOURCE(Time, "test.resource_param.Time", delta);

struct Score {
    int value;
};
AYLO_ECS_RESOURCE(Score, "test.resource_param.Score", value);
}  // namespace aylo::test::resource_param

namespace {
using aylo::ecs::NonSend;
using aylo::ecs::NonSendMut;
using aylo::ecs::Res;
using aylo::ecs::ResAppend;
using aylo::ecs::ResMut;
using aylo::ecs::ResourceAccess;
using aylo::ecs::ResourceThread;
using aylo::ecs::Tick;
using aylo::ecs::World;
using aylo::test::resource_param::Score;
using aylo::test::resource_param::Time;

// The four aliases are distinct types, so a signature tells what a system reads, writes, and where it runs.
static_assert(!std::is_same_v<Res<Time>, ResMut<Time>>);
static_assert(!std::is_same_v<Res<Time>, NonSend<Time>>);
static_assert(!std::is_same_v<ResMut<Time>, NonSendMut<Time>>);
static_assert(Res<Time>::kAccess == ResourceAccess::kRead && Res<Time>::kThread == ResourceThread::kAny);
static_assert(ResMut<Time>::kAccess == ResourceAccess::kWrite && ResMut<Time>::kThread == ResourceThread::kAny);
static_assert(NonSend<Time>::kAccess == ResourceAccess::kRead && NonSend<Time>::kThread == ResourceThread::kMain);
static_assert(NonSendMut<Time>::kAccess == ResourceAccess::kWrite &&
              NonSendMut<Time>::kThread == ResourceThread::kMain);

static_assert(ResAppend<Time>::kAccess == ResourceAccess::kAppend && ResAppend<Time>::kThread == ResourceThread::kAny);
static_assert(ResAppend<Time>::kWrites && ResMut<Time>::kWrites && !Res<Time>::kWrites);

// A read parameter gives read-only access; a written one gives write access.
static_assert(std::is_same_v<decltype(std::declval<Res<Time>&>().operator->()), const Time*>);
static_assert(std::is_same_v<decltype(*std::declval<Res<Time>&>()), const Time&>);
static_assert(std::is_same_v<decltype(std::declval<ResMut<Time>&>().operator->()), Time*>);
static_assert(std::is_same_v<decltype(*std::declval<ResMut<Time>&>()), Time&>);
}  // namespace

TEST(ResourceParam, ReadsTheValue) {
    World world;
    world.getResources().insert(Time{.delta = 0.5F});
    Res<Time> time{world.getResources(), 0, world.getChangeTick()};
    EXPECT_EQ(time->delta, 0.5F);
    EXPECT_EQ((*time).delta, 0.5F);
    EXPECT_EQ(time.get().delta, 0.5F);
}

TEST(ResourceParam, WritesAndMarksChangedOnlyWhenWritten) {
    World world;
    world.getResources().insert(Score{.value = 1});
    world.advanceChangeTick(3);
    const Tick thisRun = world.getChangeTick();

    ResMut<Score> score{world.getResources(), 0, thisRun};
    EXPECT_EQ(score.get().value, 1);
    EXPECT_EQ(world.getResources().getTicks<Score>()->changed, 0U);  // get() never marks
    score->value += 10;
    EXPECT_EQ(world.getResources().get<Score>()->value, 11);
    EXPECT_EQ(world.getResources().getTicks<Score>()->changed, thisRun);
}

TEST(ResourceParam, TellsWhetherTheValueWasAddedOrChangedSinceTheLastRun) {
    World world;
    world.getResources().insert(Score{.value = 1});  // tick 0
    world.advanceChangeTick();
    const Tick lastRun = world.getChangeTick();
    world.advanceChangeTick();

    const Res<Score> before{world.getResources(), lastRun, world.getChangeTick()};
    EXPECT_FALSE(before.isAdded());
    EXPECT_FALSE(before.isChanged());

    world.getResources().getMut<Score>()->value = 2;  // written after lastRun
    const Res<Score> after{world.getResources(), lastRun, world.getChangeTick()};
    EXPECT_FALSE(after.isAdded());
    EXPECT_TRUE(after.isChanged());

    const Res<Score> fresh{world.getResources(), 0, world.getChangeTick()};  // a first run sees what came after 0
    EXPECT_TRUE(fresh.isChanged());
}

TEST(ResourceParam, ThrowsForAMissingResourceNamingIt) {
    World world;
    try {
        const Res<Time> time{world.getResources(), 0, world.getChangeTick()};
        FAIL() << "Res did not throw";
    } catch (const aylo::ecs::exceptions::MissingResourceException& error) {
        EXPECT_NE(std::string_view{error.what()}.find("test.resource_param.Time"), std::string_view::npos);
    }
    EXPECT_THROW((ResMut<Time>{world.getResources(), 0, world.getChangeTick()}),
                 aylo::ecs::exceptions::MissingResourceException);
}

TEST(ResourceParam, NonSendParametersReadAndWriteLikeTheOthers) {
    World world;
    world.getResources().insertNonSend(Score{.value = 5});
    NonSendMut<Score> score{world.getResources(), 0, world.getChangeTick()};
    score->value = 6;
    const NonSend<Score> view{world.getResources(), 0, world.getChangeTick()};
    EXPECT_EQ(view.get().value, 6);
}
