/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DynamicQueriesTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/exceptions/QueryException.hpp"
#include "aylo/ecs/exceptions/ScheduleException.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryBuilder.hpp"
#include "aylo/ecs/query/QueryMatch.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/schedule/Schedules.hpp"
#include "aylo/ecs/system/DynamicQueries.hpp"
#include "aylo/ecs/system/QueryList.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/System.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::test::dynamic_queries {
struct Health {
    int value;
};
AYLO_ECS_COMPONENT(Health, "test.dynamicQueries.Health", value);

/// @brief What the systems under test saw.
struct Seen {
    int sum;      ///< Sum of the values read.
    int matches;  ///< Number of matches visited.
};
AYLO_ECS_RESOURCE(Seen, "test.dynamicQueries.Seen", sum, matches);
}  // namespace aylo::test::dynamic_queries

namespace {
using aylo::ecs::ComponentId;
using aylo::ecs::DynamicQueries;
using aylo::ecs::QueryBuilder;
using aylo::ecs::QueryList;
using aylo::ecs::QueryMatch;
using aylo::ecs::ResMut;
using aylo::ecs::System;
using aylo::ecs::World;
using aylo::test::dynamic_queries::Health;
using aylo::test::dynamic_queries::Seen;

/// @return The int at the start of `bytes`.
int intOf(std::span<const std::byte> bytes) {
    int value = 0;
    std::memcpy(&value, bytes.data(), sizeof(int));
    return value;
}

/// @brief Adds every value of the first data term of every query to Seen.
void sumAll(DynamicQueries queries, ResMut<Seen> seen) {
    for (std::size_t index = 0; index < queries.getCount(); ++index) {
        for (const QueryMatch& match : queries.iter(index)) {
            seen->sum += intOf(match.get(0));
            ++seen->matches;
        }
    }
}

/// @brief Adds 1 to the first data term of query 0, if it may write it.
void increment(DynamicQueries queries) {
    for (QueryMatch& match : queries.iter(0)) {
        const std::span<std::byte> bytes = match.getMut(0);
        if (!bytes.empty()) {
            const int value = intOf(bytes) + 1;
            std::memcpy(bytes.data(), &value, sizeof(int));
        }
    }
}

/// @brief Counts the matches of query 0 since tick 3, whatever the system's last run.
void countSinceThree(DynamicQueries queries, ResMut<Seen> seen) {
    for (const QueryMatch& match : queries.iterSince(0, 3)) {
        static_cast<void>(match);
        ++seen->matches;
    }
}

void writeHealth(aylo::ecs::Query<Health> healths) {
    for (auto [entity, health] : healths) {
        static_cast<void>(entity);
        static_cast<void>(health);
    }
}

/// @brief A world with two Health entities (1 and 2) and the Seen resource.
class DynamicQueriesTest : public ::testing::Test {
  protected:
    void SetUp() override {
        world.getResources().insert(Seen{.sum = 0, .matches = 0});
        world.spawn(Health{.value = 1});
        world.spawn(Health{.value = 2});
    }

    /// @return What the systems saw.
    [[nodiscard]] const Seen& seen() const { return world.getResources().require<Seen>(); }

    World world;                                                       ///< The world under test.
    ComponentId health{world.getComponents().registerType<Health>()};  ///< Health's id.
};
}  // namespace

TEST_F(DynamicQueriesTest, RunsTheConfiguredQueries) {
    System system{world, "test.sumAll", &sumAll, QueryList{{QueryBuilder{}.read(health)}}};
    system.run();
    EXPECT_EQ(seen().sum, 3);
    EXPECT_EQ(seen().matches, 2);
}

TEST_F(DynamicQueriesTest, KeepsTheQueriesInTheirOrder) {
    world.spawn(Health{.value = 10});
    const ComponentId shield = *world.getComponents().registerType(aylo::ecs::ComponentInfo{
        .type = {.name = "test.dynamicQueries.Shield",
                 .size = sizeof(int),
                 .alignment = alignof(int),
                 .fields = {{.name = "strength", .type = aylo::ecs::FieldType::kI32, .offset = 0}}}});
    const int strength = 100;
    ASSERT_TRUE(world.getComponents().add(world.spawn(), shield, std::as_bytes(std::span<const int>{&strength, 1})));
    System system{world, "test.sumAll", &sumAll, QueryList{{QueryBuilder{}.read(health), QueryBuilder{}.read(shield)}}};
    system.run();
    EXPECT_EQ(seen().sum, 1 + 2 + 10 + 100);  // a C++ component and one registered at run time
    EXPECT_EQ(seen().matches, 4);
}

TEST_F(DynamicQueriesTest, DeclaresTheAccessOfEveryQuery) {
    const System reader{world, "test.reader", &sumAll, QueryList{{QueryBuilder{}.read(health)}}};
    const System writer{world, "test.writer", &increment, QueryList{{QueryBuilder{}.write(health)}}};
    EXPECT_EQ(reader.getAccess().getComponents().size(), 1U);
    EXPECT_FALSE(reader.getAccess().getComponents().front().write);
    EXPECT_EQ(writer.getAccess().getComponents().front().id, health);
    EXPECT_TRUE(writer.getAccess().getComponents().front().write);
}

TEST_F(DynamicQueriesTest, IsOrderedAgainstTypedQueries) {
    aylo::ecs::Schedules schedules{world, aylo::ecs::RunMode::kStandalone};
    schedules.addSystem(aylo::ecs::Phase::kUpdate, "test.dynamicReader", &sumAll,
                        QueryList{{QueryBuilder{}.read(health)}});
    schedules.addSystem(aylo::ecs::Phase::kUpdate, "test.typedWriter", &writeHealth);
    try {
        schedules.build();
        FAIL() << "the ambiguity was not reported";
    } catch (const aylo::ecs::exceptions::ScheduleException& error) {
        EXPECT_NE(std::string_view{error.what()}.find("'test.dynamicReader' and 'test.typedWriter' both use component"),
                  std::string_view::npos)
            << error.what();
    }

    aylo::ecs::Schedules ordered{world, aylo::ecs::RunMode::kStandalone};
    ordered
        .addSystem(aylo::ecs::Phase::kUpdate, "test.dynamicReader", &sumAll, QueryList{{QueryBuilder{}.read(health)}})
        .after("test.typedWriter");
    ordered.addSystem(aylo::ecs::Phase::kUpdate, "test.typedWriter", &writeHealth);
    EXPECT_NO_THROW(ordered.build());
}

TEST_F(DynamicQueriesTest, WritesOnlyWhatItDeclared) {
    System writer{world, "test.writer", &increment, QueryList{{QueryBuilder{}.write(health)}}};
    writer.run();
    System reader{world, "test.reader", &increment, QueryList{{QueryBuilder{}.read(health)}}};
    reader.run();  // a read term refuses write access: nothing changes
    System sum{world, "test.sumAll", &sumAll, QueryList{{QueryBuilder{}.read(health)}}};
    sum.run();
    EXPECT_EQ(seen().sum, 2 + 3);
}

TEST_F(DynamicQueriesTest, CountsChangesSinceTheSystemLastRan) {
    System system{world, "test.sumChanged", &sumAll, QueryList{{QueryBuilder{}.read(health).changed(health)}}};
    system.run();
    EXPECT_EQ(seen().matches, 2);  // the first run sees everything
    system.run();
    EXPECT_EQ(seen().matches, 2);  // nothing changed since
    System writer{world, "test.writer", &increment, QueryList{{QueryBuilder{}.write(health)}}};
    writer.run();
    system.run();
    EXPECT_EQ(seen().matches, 4);  // both written after its last run
}

TEST_F(DynamicQueriesTest, CountsChangesSinceAnyTick) {
    world.advanceChangeTick(5);
    System writer{world, "test.writer", &increment, QueryList{{QueryBuilder{}.write(health)}}};
    writer.run();
    System counter{world, "test.countSinceThree", &countSinceThree,
                   QueryList{{QueryBuilder{}.read(health).changed(health)}}};
    counter.run();
    EXPECT_EQ(seen().matches, 2);  // written at 5, after tick 3
    counter.run();
    EXPECT_EQ(seen().matches, 4);  // still since tick 3, not since the system's last run
}

TEST_F(DynamicQueriesTest, ThrowsWhenAQueryCannotBeBuilt) {
    try {
        const System system{world, "test.sumAll", &sumAll,
                            QueryList{{QueryBuilder{}.read(health), QueryBuilder{}.read(ComponentId{42})}}};
        FAIL() << "no QueryException";
    } catch (const aylo::ecs::exceptions::QueryException& error) {
        const std::string_view message{error.what()};
        EXPECT_NE(message.find("Query 1 of the system cannot be built"), std::string_view::npos) << message;
        EXPECT_NE(message.find("42"), std::string_view::npos) << message;
    }
}

TEST_F(DynamicQueriesTest, ShowsItsQueries) {
    const std::vector<aylo::ecs::DynamicQuery> queries{*QueryBuilder{}.read(health).build(world.getComponents())};
    const DynamicQueries view{queries, 7};
    EXPECT_EQ(view.getCount(), 1U);
    EXPECT_EQ(&view.getQuery(0), &queries.front());
    EXPECT_EQ(view.getLastRun(), 7U);
}
