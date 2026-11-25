/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DiscoveredAccessTests
*/

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/call/Calls.hpp"
#include "aylo/ecs/call/DynamicCalls.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/ecs/exceptions/CallException.hpp"
#include "aylo/ecs/exceptions/ResourceExceptions.hpp"
#include "aylo/ecs/exceptions/ScheduleException.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/resource/ResourceInfo.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/schedule/Schedule.hpp"
#include "aylo/ecs/system/DynamicEvents.hpp"
#include "aylo/ecs/system/DynamicResources.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/ScriptCallable.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/system/System.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::test::discovered {
struct Score {
    int value{0};
};
AYLO_ECS_RESOURCE(Score, "test.discovered.Score", value);

struct Health {
    int value{0};
};
AYLO_ECS_COMPONENT(Health, "test.discovered.Health", value);

struct Ping {
    int value{0};
};
AYLO_ECS_EVENT(Ping, "test.discovered.Ping", value);

/// @brief Adds one to the score.
void addPoint(ecs::ResMut<Score> score) { ++score->value; }

/// @brief Writes every Health.
void heal(ecs::Query<Health> healths) {
    for (auto [entity, health] : healths) {
        ++health->value;
    }
}

/// @brief Writes the score: declares it.
void writeScore(ecs::ResMut<Score> /*score*/) {}
}  // namespace aylo::test::discovered

namespace {
using aylo::ecs::App;
using aylo::ecs::CallId;
using aylo::ecs::CallList;
using aylo::ecs::DynamicCalls;
using aylo::ecs::DynamicEvents;
using aylo::ecs::DynamicResources;
using aylo::ecs::EventId;
using aylo::ecs::EventList;
using aylo::ecs::Phase;
using aylo::ecs::ResourceId;
using aylo::ecs::ResourceList;
using aylo::ecs::RunMode;
using aylo::ecs::Schedule;
using aylo::ecs::ScriptCallable;
using aylo::ecs::Side;
using aylo::ecs::System;
using aylo::ecs::World;
using aylo::test::discovered::Health;
using aylo::test::discovered::Ping;
using aylo::test::discovered::Score;

/// @return The int at the start of `bytes`.
int intOf(std::span<const std::byte> bytes) {
    int value = 0;
    std::memcpy(&value, bytes.data(), sizeof(int));
    return value;
}

/// @return The order of a schedule, as strings, to compare it.
std::vector<std::string> orderOf(const Schedule& schedule) {
    std::vector<std::string> names;
    for (const std::string_view name : schedule.getOrder()) {
        names.emplace_back(name);
    }
    return names;
}
}  // namespace

TEST(DiscoveredAccess, AResourceFoundWhileTheSystemRunsGrowsItsAccess) {
    World world;
    world.getResources().insert(Score{.value = 4});
    const ResourceId score = world.getResources().registerType<Score>();
    int seen = 0;
    System system{world, "test.discover", ScriptCallable{[score, &seen](DynamicResources resources) {
                      const std::size_t index = resources.add(score, false);
                      EXPECT_EQ(resources.add(score, false), index);  // found once
                      seen = intOf(resources.get(index));
                      EXPECT_TRUE(resources.getMut(index).empty());  // read only, so far
                  }},
                  ResourceList{}};
    EXPECT_TRUE(system.getAccess().getResources().empty());

    system.run();

    EXPECT_EQ(seen, 4);
    ASSERT_EQ(system.getAccess().getResources().size(), 1U);
    EXPECT_FALSE(system.getAccess().getResources().front().write);
    EXPECT_TRUE(system.getDeclaredAccess().getResources().empty());
    EXPECT_TRUE(system.takeAccessGrowth());
    EXPECT_FALSE(system.takeAccessGrowth());  // told once
}

TEST(DiscoveredAccess, AResourceReadThenWrittenBecomesAWrite) {
    World world;
    world.getResources().insert(Score{.value = 4});
    const ResourceId score = world.getResources().registerType<Score>();
    System system{world, "test.discover", ScriptCallable{[score](DynamicResources resources) {
                      const std::size_t index = resources.add(score, false);
                      EXPECT_EQ(resources.add(score, true), index);
                      const std::span<std::byte> bytes = resources.getMut(index);
                      ASSERT_FALSE(bytes.empty());
                      const int value = 9;
                      std::memcpy(bytes.data(), &value, sizeof(int));
                  }},
                  ResourceList{}};

    system.run();

    EXPECT_EQ(world.getResources().require<Score>().value, 9);
    ASSERT_EQ(system.getAccess().getResources().size(), 1U);
    EXPECT_TRUE(system.getAccess().getResources().front().write);
}

TEST(DiscoveredAccess, AResourceCannotBeAddedOutsideASystemRun) {
    World world;
    const ResourceId score = world.getResources().registerType<Score>();
    ResourceList list;
    const DynamicResources resources{world.getResources(), list, nullptr};
    EXPECT_THROW(static_cast<void>(resources.add(score, false)), aylo::ecs::exceptions::ResourceRegistrationException);
    EXPECT_THROW(static_cast<void>(resources.add(ResourceId{99}, false)),
                 aylo::ecs::exceptions::ResourceRegistrationException);
}

TEST(DiscoveredAccess, AnEventFoundWhileTheSystemRunsReadsWhatTheQueueHolds) {
    World world;
    const EventId ping = world.getEvents().registerType<Ping>();
    world.getEvents().send(Ping{.value = 3});
    int sum = 0;
    System system{world, "test.discover", ScriptCallable{[ping, &sum](const DynamicEvents events) {
                      const std::size_t index = events.add(ping, true, false);
                      for (const std::span<const std::byte> bytes : events.read(index)) {
                          sum += intOf(bytes);
                      }
                      EXPECT_EQ(events.add(ping, false, true), index);  // now sent too
                      const int value = 5;
                      EXPECT_TRUE(events.send(index, std::as_bytes(std::span<const int>{&value, 1})));
                  }},
                  EventList{}};

    system.run();

    EXPECT_EQ(sum, 3);
    ASSERT_EQ(system.getAccess().getEvents().size(), 1U);
    EXPECT_TRUE(system.getAccess().getEvents().front().write);
}

TEST(DiscoveredAccess, ACallFoundWhileTheSystemRunsIsBoundAndAddsItsAccess) {
    App app;
    World& world = app.getWorld();
    world.getResources().insert(Score{.value = 1});
    const CallId add = app.addCall("test.addPoint", &aylo::test::discovered::addPoint);
    System system{world, "test.discover", ScriptCallable{[add](const DynamicCalls calls) {
                      EXPECT_FALSE(calls.find(add).has_value());
                      const std::size_t index = calls.add(add);
                      EXPECT_EQ(calls.add(add), index);
                      calls.invoke(index, {}, {});
                  }},
                  CallList{}};

    system.run();

    EXPECT_EQ(world.getResources().require<Score>().value, 2);
    ASSERT_EQ(system.getAccess().getResources().size(), 1U);
    EXPECT_TRUE(system.getAccess().getResources().front().write);
    EXPECT_TRUE(system.getDeclaredAccess().getResources().empty());
}

TEST(DiscoveredAccess, AClientSystemCannotFindACallThatWritesReplicatedData) {
    App app;
    World& world = app.getWorld();
    world.getComponents().registerType<Health>({.replication = aylo::ecs::ReplicationMode::kEveryChange});
    const CallId heal = app.addCall("test.heal", &aylo::test::discovered::heal);
    std::string message;
    System system{world, "test.discover", ScriptCallable{[heal, &message](const DynamicCalls calls) {
                      try {
                          static_cast<void>(calls.add(heal));
                      } catch (const aylo::ecs::exceptions::CallException& error) {
                          message = error.what();
                      }
                      EXPECT_EQ(calls.getCount(), 0U);
                  }},
                  CallList{}};
    system.setSide(Side::kClient);

    system.run();

    EXPECT_THAT(message, testing::HasSubstr("'test.heal'"));
    EXPECT_THAT(message, testing::HasSubstr("test.discovered.Health"));
    EXPECT_THAT(message, testing::HasSubstr("replicated"));
    EXPECT_FALSE(system.takeAccessGrowth());
}

TEST(DiscoveredAccess, AnAmbiguityFoundWhileRunningIsAWarningOnce) {
    World world;
    world.getResources().insert(Score{});
    const ResourceId score = world.getResources().registerType<Score>();
    Schedule schedule{world, RunMode::kServer, Phase::kUpdate};
    schedule.addSystem("a.writer", &aylo::test::discovered::writeScore);
    schedule.addSystem(System{world, "b.finder", ScriptCallable{[score](DynamicResources resources) {
                                  static_cast<void>(resources.add(score, false));
                              }},
                              ResourceList{}});

    testing::internal::CaptureStderr();
    EXPECT_NO_THROW(schedule.run());
    EXPECT_NO_THROW(schedule.run());  // built again with what was found: still no error
    const std::string output = testing::internal::GetCapturedStderr();

    EXPECT_THAT(output, testing::HasSubstr("'a.writer' and 'b.finder' both use resource 'test.discovered.Score'"));
    EXPECT_THAT(output, testing::HasSubstr("before / after"));
    EXPECT_EQ(output.find("b.finder"), output.rfind("b.finder"));  // written once
}

TEST(DiscoveredAccess, ADeclaredAmbiguityIsStillAnError) {
    World world;
    Schedule schedule{world, RunMode::kServer, Phase::kUpdate};
    schedule.addSystem("a", &aylo::test::discovered::writeScore);
    schedule.addSystem("b", &aylo::test::discovered::writeScore);
    EXPECT_THROW(schedule.build(), aylo::ecs::exceptions::ScheduleException);
}

TEST(DiscoveredAccess, AnEventSentWithoutDeclaringItIsListedAsWritten) {
    World world;
    const EventId ping = world.getEvents().registerType<Ping>();
    Schedule schedule{world, RunMode::kServer, Phase::kFixedUpdate};
    schedule.addSystem(System{world, "send", ScriptCallable{[ping](const DynamicEvents events) {
                                  const int value = 1;
                                  static_cast<void>(events.send(events.add(ping, false, true),
                                                                std::as_bytes(std::span<const int>{&value, 1})));
                              }},
                              EventList{}});
    schedule.build();
    EXPECT_TRUE(schedule.getWrittenEvents().empty());

    schedule.run();

    EXPECT_THAT(schedule.getWrittenEvents(), testing::ElementsAre(ping));
}

TEST(DiscoveredAccess, StandaloneOrdersTheServerFirstOnceItFindsTheClientsData) {
    World world;
    world.getResources().insert(Score{});
    const ResourceId score = world.getResources().registerType<Score>();
    Schedule schedule{world, RunMode::kStandalone, Phase::kUpdate};
    schedule.addSystem("a.client", &aylo::test::discovered::writeScore).setSide(Side::kClient);
    schedule
        .addSystem(System{world, "b.server", ScriptCallable{[score](DynamicResources resources) {
                              static_cast<void>(resources.add(score, true));
                          }},
                          ResourceList{}})
        .setSide(Side::kServer);
    schedule.build();
    EXPECT_THAT(orderOf(schedule), testing::ElementsAre("a.client", "b.server"));  // by name: nothing known yet

    testing::internal::CaptureStderr();
    schedule.run();
    schedule.run();
    const std::string output = testing::internal::GetCapturedStderr();

    EXPECT_THAT(orderOf(schedule), testing::ElementsAre("b.server", "a.client"));
    EXPECT_EQ(output, "");  // ordered, so not ambiguous
}
