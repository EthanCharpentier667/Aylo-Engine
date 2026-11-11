/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SystemParamTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <vector>

#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryTerms.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/system/EventReader.hpp"
#include "aylo/ecs/system/EventWriter.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/system/SystemParam.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::test::system_param {
struct Position {
    float x;
    float y;
};
AYLO_ECS_COMPONENT(Position, "test.system_param.Position", x, y);

struct Velocity {
    float x;
    float y;
};
AYLO_ECS_COMPONENT(Velocity, "test.system_param.Velocity", x, y);

struct Time {
    float delta;
};
AYLO_ECS_RESOURCE(Time, "test.system_param.Time", delta);

struct Renderer {
    int frames;
};
AYLO_ECS_RESOURCE(Renderer, "test.system_param.Renderer", frames);

struct Collision {
    std::uint32_t a;
};
AYLO_ECS_EVENT(Collision, "test.system_param.Collision", a);
}  // namespace aylo::test::system_param

namespace {
using aylo::ecs::Commands;
using aylo::ecs::EventReader;
using aylo::ecs::EventWriter;
using aylo::ecs::NonSend;
using aylo::ecs::NonSendMut;
using aylo::ecs::Query;
using aylo::ecs::Res;
using aylo::ecs::ResMut;
using aylo::ecs::SystemAccess;
using aylo::ecs::SystemContext;
using aylo::ecs::SystemParam;
using aylo::ecs::SystemParameter;
using aylo::ecs::TypeAccess;
using aylo::ecs::World;
using aylo::test::system_param::Collision;
using aylo::test::system_param::Position;
using aylo::test::system_param::Renderer;
using aylo::test::system_param::Time;
using aylo::test::system_param::Velocity;

// The accepted parameter types, exactly as written.
static_assert(SystemParameter<Query<Position, const Velocity>>);
static_assert(SystemParameter<Res<Time>>);
static_assert(SystemParameter<ResMut<Time>>);
static_assert(SystemParameter<NonSend<Renderer>>);
static_assert(SystemParameter<NonSendMut<Renderer>>);
static_assert(SystemParameter<Commands&>);
static_assert(SystemParameter<EventReader<Collision>>);
static_assert(SystemParameter<EventWriter<Collision>>);

// Anything else is refused.
static_assert(!SystemParameter<int>);
static_assert(!SystemParameter<Position>);
static_assert(!SystemParameter<World&>);
static_assert(!SystemParameter<Query<Position>&>);  // a Query is a view: take it by value
static_assert(!SystemParameter<Res<Time>&>);
static_assert(!SystemParameter<Commands>);  // the buffer belongs to the system: take it by reference
static_assert(!SystemParameter<const Commands&>);

/// @return The accesses as a vector, to compare them.
std::vector<TypeAccess> toVector(std::span<const TypeAccess> accesses) { return {accesses.begin(), accesses.end()}; }
}  // namespace

TEST(SystemParam, BuildsAQueryViewAndAddsItsComponents) {
    World world;
    world.spawn(Position{}, Velocity{});
    Commands commands{world};
    using Traits = SystemParam<Query<Position, const Velocity>>;
    Traits::State state = Traits::init(world);
    const auto position = world.getComponents().find("test.system_param.Position");
    const auto velocity = world.getComponents().find("test.system_param.Velocity");
    ASSERT_TRUE(position && velocity);  // registered by init()

    Query<Position, const Velocity> query = Traits::get(state, SystemContext{&world, &commands, 7, 9});
    EXPECT_EQ(query.getLastRun(), 7U);
    EXPECT_EQ(std::ranges::distance(query), 1);

    SystemAccess access;
    Traits::addAccess(state, access);
    EXPECT_EQ(toVector(access.getComponents()),
              (std::vector<TypeAccess>{{.id = *position, .write = true}, {.id = *velocity, .write = false}}));
    EXPECT_FALSE(access.isMainThread());
}

TEST(SystemParam, BuildsResourcesAndAddsTheirAccess) {
    World world;
    world.getResources().insert(Time{.delta = 0.5F});
    Commands commands{world};
    const SystemContext context{&world, &commands, 0, world.getChangeTick()};

    using Read = SystemParam<Res<Time>>;
    Read::State read = Read::init(world);
    EXPECT_EQ(Read::get(read, context).get().delta, 0.5F);
    using Write = SystemParam<ResMut<Time>>;
    Write::State write = Write::init(world);

    SystemAccess access;
    Read::addAccess(read, access);
    Write::addAccess(write, access);
    const aylo::ecs::ResourceId time = *world.getResources().find("test.system_param.Time");
    EXPECT_EQ(toVector(access.getResources()), (std::vector<TypeAccess>{{.id = time, .write = true}}));
    EXPECT_FALSE(access.isMainThread());
}

TEST(SystemParam, NonSendRegistersAMainThreadResourceAndRequiresTheMainThread) {
    World world;
    using Traits = SystemParam<NonSendMut<Renderer>>;
    Traits::State state = Traits::init(world);  // registered before the renderer exists, as main-thread only
    EXPECT_TRUE(world.getResources().getRegistry().find<Renderer>()->nonSend);
    EXPECT_NO_THROW(world.getResources().insertNonSend(Renderer{.frames = 0}));

    SystemAccess access;
    Traits::addAccess(state, access);
    EXPECT_EQ(toVector(access.getResources()), (std::vector<TypeAccess>{{.id = state, .write = true}}));
    EXPECT_TRUE(access.isMainThread());
}

TEST(SystemParam, GivesTheSystemItsCommands) {
    World world;
    Commands commands{world};
    using Traits = SystemParam<Commands&>;
    Traits::State state = Traits::init(world);
    EXPECT_EQ(&Traits::get(state, SystemContext{&world, &commands, 0, 0}), &commands);

    SystemAccess access;
    Traits::addAccess(state, access);
    EXPECT_TRUE(access.getComponents().empty());
    EXPECT_TRUE(access.getResources().empty());
}

TEST(SystemParam, KeepsAReaderCursorAndAddsEventAccess) {
    World world;
    Commands commands{world};
    using Reader = SystemParam<EventReader<Collision>>;
    using Writer = SystemParam<EventWriter<Collision>>;
    Reader::State reader = Reader::init(world);
    Writer::State writer = Writer::init(world);
    const SystemContext context{&world, &commands, 0, 0};

    Writer::get(writer, context).send(Collision{.a = 1});
    EXPECT_EQ(Reader::get(reader, context).size(), 1U);
    EXPECT_EQ(Reader::get(reader, context).size(), 0U);  // the cursor kept in the state moved on

    SystemAccess access;
    Reader::addAccess(reader, access);
    Writer::addAccess(writer, access);
    EXPECT_EQ(toVector(access.getEvents()), (std::vector<TypeAccess>{{.id = reader.id, .write = true}}));
}
