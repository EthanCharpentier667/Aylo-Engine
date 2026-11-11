/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** WorldEventTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/event/EventCursor.hpp"
#include "aylo/ecs/event/EventInfo.hpp"
#include "aylo/ecs/event/EventRange.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/ecs/exceptions/TypeExceptions.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/storage/ViewAs.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::test::world_event {
struct Hit {
    std::int32_t damage;
};
AYLO_ECS_EVENT(Hit, "test.worldevt.Hit", damage);

struct Packet {
    std::unique_ptr<std::int32_t> payload;
};
AYLO_ECS_EVENT(Packet, "test.worldevt.Packet");

struct GameOver {};
AYLO_ECS_EVENT(GameOver, "test.worldevt.GameOver");

/// @brief A component, a resource and an event that would share one name.
struct Score {
    std::int32_t value;
};
AYLO_ECS_COMPONENT(Score, "test.worldevt.Score", value);

struct Clock {
    float time;
};
AYLO_ECS_RESOURCE(Clock, "test.worldevt.Clock");

struct ScoreEvent {
    std::int32_t value;
};
AYLO_ECS_EVENT(ScoreEvent, "test.worldevt.Score", value);

struct ClockEvent {
    float time;
};
AYLO_ECS_EVENT(ClockEvent, "test.worldevt.Clock");

struct Spawned {
    std::int32_t id;
};
AYLO_ECS_EVENT(Spawned, "test.worldevt.Spawned", id);

struct SpawnedComponent {
    std::int32_t id;
};
AYLO_ECS_COMPONENT(SpawnedComponent, "test.worldevt.Spawned", id);

struct SpawnedResource {
    std::int32_t id;
};
AYLO_ECS_RESOURCE(SpawnedResource, "test.worldevt.Spawned");
}  // namespace aylo::test::world_event

namespace wev = aylo::test::world_event;

namespace {
/// @brief The C++ view of a Boss value, which only exists as a run-time description in the world.
struct BossValue {
    std::int32_t boss;
};

/// @return A run-time description, as a Luau script would provide it.
aylo::ecs::EventInfo makeBossDefeated() {
    return aylo::ecs::EventInfo{.type = {.name = "test.worldevt.BossDefeated", .size = 4, .alignment = 4}};
}

/// @return The damage of every Hit the cursor has not read yet.
std::vector<std::int32_t> readDamages(const aylo::ecs::World& world, aylo::ecs::EventCursor& cursor) {
    std::vector<std::int32_t> damages;
    for (const wev::Hit& hit : world.getEvents().read<wev::Hit>(cursor)) {
        damages.push_back(hit.damage);
    }
    return damages;
}
}  // namespace

TEST(WorldEvent, StartsWithoutEvents) {
    const aylo::ecs::World world;
    aylo::ecs::EventCursor cursor;
    EXPECT_EQ(world.getEvents().getRegistry().getCount(), 0U);
    EXPECT_EQ(world.getEvents().getCount<wev::Hit>(), 0U);
    EXPECT_TRUE(world.getEvents().read<wev::Hit>(cursor).empty());
    EXPECT_FALSE(world.getEvents().find("test.worldevt.Hit"));
}

TEST(WorldEvent, SendsAndReadsInOrder) {
    aylo::ecs::World world;
    world.getEvents().send(wev::Hit{.damage = 1});
    world.getEvents().send(wev::Hit{.damage = 2});
    aylo::ecs::EventCursor cursor;
    EXPECT_EQ(readDamages(world, cursor), (std::vector<std::int32_t>{1, 2}));
    EXPECT_TRUE(world.getEvents().find("test.worldevt.Hit"));
}

TEST(WorldEvent, AReaderSeesEachEventOnce) {
    aylo::ecs::World world;
    aylo::ecs::EventCursor cursor;
    world.getEvents().send(wev::Hit{.damage = 1});
    EXPECT_EQ(readDamages(world, cursor), (std::vector<std::int32_t>{1}));
    EXPECT_TRUE(readDamages(world, cursor).empty());
    world.getEvents().send(wev::Hit{.damage = 2});
    EXPECT_EQ(readDamages(world, cursor), (std::vector<std::int32_t>{2}));
}

TEST(WorldEvent, ReadersAreIndependent) {
    aylo::ecs::World world;
    aylo::ecs::EventCursor early;
    aylo::ecs::EventCursor late;
    world.getEvents().send(wev::Hit{.damage = 1});
    EXPECT_EQ(readDamages(world, early), (std::vector<std::int32_t>{1}));
    world.getEvents().send(wev::Hit{.damage = 2});
    EXPECT_EQ(readDamages(world, late), (std::vector<std::int32_t>{1, 2}));
    EXPECT_EQ(readDamages(world, early), (std::vector<std::int32_t>{2}));
}

TEST(WorldEvent, AnEventIsReadableDuringTheNextUpdateThenDropped) {
    aylo::ecs::World world;
    world.getEvents().send(wev::Hit{.damage = 1});
    world.getEvents().update();
    aylo::ecs::EventCursor nextUpdate;
    EXPECT_EQ(readDamages(world, nextUpdate), (std::vector<std::int32_t>{1}));
    world.getEvents().update();
    aylo::ecs::EventCursor tooLate;
    EXPECT_TRUE(readDamages(world, tooLate).empty());
    EXPECT_EQ(world.getEvents().getCount<wev::Hit>(), 0U);
}

TEST(WorldEvent, AReaderThatRunsBeforeTheWriterSeesItNextUpdate) {
    aylo::ecs::World world;
    aylo::ecs::EventCursor reader;
    EXPECT_TRUE(readDamages(world, reader).empty());  // runs first, before the writer of this update
    world.getEvents().send(wev::Hit{.damage = 3});
    world.getEvents().update();
    EXPECT_EQ(readDamages(world, reader), (std::vector<std::int32_t>{3}));
}

TEST(WorldEvent, AReaderThatSkipsAnUpdateMissesTheDroppedEvents) {
    aylo::ecs::World world;
    aylo::ecs::EventCursor reader;
    world.getEvents().send(wev::Hit{.damage = 1});
    world.getEvents().update();
    world.getEvents().send(wev::Hit{.damage = 2});
    world.getEvents().update();
    EXPECT_EQ(readDamages(world, reader), (std::vector<std::int32_t>{2}));
}

TEST(WorldEvent, UpdatesOneQueueAtATime) {
    aylo::ecs::World world;
    world.getEvents().send(wev::Hit{.damage = 1});
    world.getEvents().send(wev::GameOver{});
    EXPECT_TRUE(world.getEvents().update<wev::Hit>());
    EXPECT_TRUE(world.getEvents().update<wev::Hit>());
    EXPECT_EQ(world.getEvents().getCount<wev::Hit>(), 0U);
    EXPECT_EQ(world.getEvents().getCount<wev::GameOver>(), 1U);
    EXPECT_FALSE(world.getEvents().update<wev::Packet>());
}

TEST(WorldEvent, ClearDropsTheEventsAndKeepsCursorsValid) {
    aylo::ecs::World world;
    aylo::ecs::EventCursor cursor;
    world.getEvents().send(wev::Hit{.damage = 1});
    EXPECT_TRUE(world.getEvents().clear<wev::Hit>());
    EXPECT_TRUE(readDamages(world, cursor).empty());
    world.getEvents().send(wev::Hit{.damage = 2});
    EXPECT_EQ(readDamages(world, cursor), (std::vector<std::int32_t>{2}));
    EXPECT_FALSE(world.getEvents().clear<wev::Packet>());
}

TEST(WorldEvent, CarriesMoveOnlyEvents) {
    aylo::ecs::World world;
    world.getEvents().send(wev::Packet{.payload = std::make_unique<std::int32_t>(42)});
    aylo::ecs::EventCursor cursor;
    const aylo::ecs::EventRange<wev::Packet> packets = world.getEvents().read<wev::Packet>(cursor);
    ASSERT_EQ(packets.size(), 1U);
    EXPECT_EQ(*packets.begin()->payload, 42);
}

TEST(WorldEvent, CountsEventsWithoutData) {
    aylo::ecs::World world;
    world.getEvents().send(wev::GameOver{});
    world.getEvents().send(wev::GameOver{});
    aylo::ecs::EventCursor cursor;
    EXPECT_EQ(world.getEvents().read<wev::GameOver>(cursor).size(), 2U);
    EXPECT_TRUE(world.getEvents().read<wev::GameOver>(cursor).empty());
}

TEST(WorldEvent, SendsAndReadsARunTimeEventById) {
    aylo::ecs::World world;
    const aylo::ecs::Result<aylo::ecs::EventId> id = world.getEvents().registerType(makeBossDefeated());
    ASSERT_TRUE(id);
    EXPECT_EQ(world.getEvents().find("test.worldevt.BossDefeated"), *id);
    const BossValue boss{.boss = 7};
    EXPECT_TRUE(world.getEvents().send(*id, std::as_bytes(std::span{&boss, 1})));
    aylo::ecs::EventCursor cursor;
    const aylo::ecs::RawEventRange events = world.getEvents().read(*id, cursor);
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(aylo::ecs::storage::viewAs<BossValue>(*events.begin())->boss, 7);
    EXPECT_EQ(world.getEvents().getCount(*id), 1U);
    EXPECT_TRUE(world.getEvents().update(*id));
    EXPECT_TRUE(world.getEvents().clear(*id));
    EXPECT_EQ(world.getEvents().getCount(*id), 0U);
}

TEST(WorldEvent, RefusesInvalidSendsById) {
    aylo::ecs::World world;
    const aylo::ecs::EventId boss = *world.getEvents().registerType(makeBossDefeated());
    const aylo::ecs::EventId packet = world.getEvents().registerType<wev::Packet>();
    const std::int64_t wrongSize = 1;
    EXPECT_FALSE(world.getEvents().send(boss, std::as_bytes(std::span{&wrongSize, 1})));
    const wev::Packet value{};
    EXPECT_FALSE(world.getEvents().send(packet, std::as_bytes(std::span{&value, 1})));
    const BossValue bossValue{.boss = 1};
    EXPECT_FALSE(world.getEvents().send(aylo::ecs::EventId{99}, std::as_bytes(std::span{&bossValue, 1})));
    EXPECT_EQ(world.getEvents().getCount(boss), 0U);
    EXPECT_EQ(world.getEvents().getCount(packet), 0U);
}

TEST(WorldEvent, AnswersNothingForAnUnknownId) {
    aylo::ecs::World world;
    const aylo::ecs::EventId unknown{5};
    aylo::ecs::EventCursor cursor{.next = 3};
    EXPECT_TRUE(world.getEvents().read(unknown, cursor).empty());
    EXPECT_EQ(cursor.next, 3U);
    EXPECT_EQ(world.getEvents().getCount(unknown), 0U);
    EXPECT_FALSE(world.getEvents().update(unknown));
    EXPECT_FALSE(world.getEvents().clear(unknown));
}

TEST(WorldEvent, RefusesAReplicatedRunTimeEvent) {
    aylo::ecs::World world;
    aylo::ecs::EventInfo info = makeBossDefeated();
    info.type.replication = aylo::ecs::ReplicationMode::kEveryChange;
    const aylo::ecs::Result<aylo::ecs::EventId> id = world.getEvents().registerType(info);
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), aylo::ecs::ErrorKind::kInvalidType);
}

TEST(WorldEvent, AnEventCannotTakeTheNameOfAComponentOrAResource) {
    aylo::ecs::World world;
    world.getComponents().registerType<wev::Score>();
    world.getResources().registerType<wev::Clock>();
    EXPECT_THROW(world.getEvents().registerType<wev::ScoreEvent>(), aylo::ecs::exceptions::TypeRegistrationException);
    EXPECT_THROW(world.getEvents().send(wev::ClockEvent{}), aylo::ecs::exceptions::TypeRegistrationException);
    aylo::ecs::EventInfo info = makeBossDefeated();
    info.type.name = "test.worldevt.Score";
    const aylo::ecs::Result<aylo::ecs::EventId> id = world.getEvents().registerType(info);
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), aylo::ecs::ErrorKind::kNameCollision);
    EXPECT_EQ(world.getEvents().getRegistry().getCount(), 0U);
}

TEST(WorldEvent, AComponentOrAResourceCannotTakeTheNameOfAnEvent) {
    aylo::ecs::World world;
    world.getEvents().registerType<wev::Spawned>();
    EXPECT_THROW(world.getComponents().registerType<wev::SpawnedComponent>(),
                 aylo::ecs::exceptions::TypeRegistrationException);
    EXPECT_THROW(world.getResources().registerType<wev::SpawnedResource>(),
                 aylo::ecs::exceptions::TypeRegistrationException);
}

TEST(WorldEvent, WorldsAreIndependent) {
    aylo::ecs::World first;
    const aylo::ecs::World second;
    first.getEvents().send(wev::Hit{.damage = 1});
    EXPECT_EQ(second.getEvents().getCount<wev::Hit>(), 0U);
    EXPECT_FALSE(second.getEvents().find("test.worldevt.Hit"));
}

TEST(WorldEvent, MovingKeepsEvents) {
    aylo::ecs::World world;
    world.getEvents().send(wev::Hit{.damage = 4});
    const aylo::ecs::World moved{std::move(world)};
    aylo::ecs::EventCursor cursor;
    EXPECT_EQ(readDamages(moved, cursor), (std::vector<std::int32_t>{4}));
}

TEST(WorldEvent, ARangeStaysValidWhenOtherEventsAreRegistered) {
    aylo::ecs::World world;
    world.getEvents().send(wev::Hit{.damage = 8});
    aylo::ecs::EventCursor cursor;
    const aylo::ecs::EventRange<wev::Hit> hits = world.getEvents().read<wev::Hit>(cursor);
    world.getEvents().registerType<wev::GameOver>();
    world.getEvents().registerType<wev::Packet>();
    ASSERT_EQ(hits.size(), 1U);
    EXPECT_EQ(hits.begin()->damage, 8);
}
