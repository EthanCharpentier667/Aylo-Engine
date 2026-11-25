/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RemovedQueryTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

#include "aylo/ecs/change/Tick.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/query/DynamicQuery.hpp"
#include "aylo/ecs/query/QueryBuilder.hpp"
#include "aylo/ecs/query/QueryIteration.hpp"
#include "aylo/ecs/query/QueryMatch.hpp"
#include "aylo/ecs/query/QueryState.hpp"
#include "aylo/ecs/query/QueryTerm.hpp"
#include "aylo/ecs/query/QueryTerms.hpp"
#include "aylo/ecs/storage/RemovalLog.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::test::removed {
struct Health {
    int value{0};
};
AYLO_ECS_COMPONENT(Health, "test.removed.Health", value);

struct Armor {
    int value{0};
};
AYLO_ECS_COMPONENT(Armor, "test.removed.Armor", value);
}  // namespace aylo::test::removed

namespace {
using aylo::ecs::Entity;
using aylo::ecs::QueryState;
using aylo::ecs::Removed;
using aylo::ecs::Tick;
using aylo::ecs::With;
using aylo::ecs::World;
using aylo::test::removed::Armor;
using aylo::test::removed::Health;

/// @return The entities a pass of `query` visits.
template <typename Query>
std::vector<Entity> visit(const Query& query, const Tick lastRun) {
    std::vector<Entity> entities;
    for (const auto& item : query.iter(lastRun)) {
        entities.push_back(std::get<0>(item));
    }
    return entities;
}

/// @return The removals kept for `T` in `world`.
template <typename T>
std::size_t keptRemovals(World& world) {
    const auto id = world.getComponents().registerType<T>();
    return world.getComponents().findPool(id)->getRemovalLog().getRemovals().size();
}
}  // namespace

TEST(RemovedQuery, ARemovalAndADespawnAreVisitedOnce) {
    World world;
    const QueryState<Removed<Health>> query{world.getComponents()};
    const Entity removed = world.spawn(Health{.value = 1});
    const Entity despawned = world.spawn(Health{.value = 2});
    const Entity kept = world.spawn(Health{.value = 3});
    Tick lastRun = world.getChangeTick();
    world.advanceChangeTick();

    EXPECT_TRUE(world.getComponents().remove<Health>(removed));
    EXPECT_TRUE(world.despawn(despawned));

    EXPECT_EQ(visit(query, lastRun), (std::vector<Entity>{removed, despawned}));
    EXPECT_TRUE(world.isAlive(removed));
    EXPECT_FALSE(world.isAlive(despawned));
    EXPECT_TRUE(world.isAlive(kept));
    lastRun = world.getChangeTick();
    world.advanceChangeTick();
    EXPECT_TRUE(visit(query, lastRun).empty());  // seen once
}

TEST(RemovedQuery, OtherTermsAreCheckedOnTheEntityAsItIsNow) {
    World world;
    const QueryState<Removed<Health>, With<Armor>> query{world.getComponents()};
    const Entity armored = world.spawn(Health{.value = 1}, Armor{.value = 1});
    const Entity bare = world.spawn(Health{.value = 1});
    const Entity healed = world.spawn(Health{.value = 1}, Armor{.value = 1});
    const Tick lastRun = world.getChangeTick();
    world.advanceChangeTick();

    static_cast<void>(world.getComponents().remove<Health>(armored));
    static_cast<void>(world.getComponents().remove<Health>(bare));
    static_cast<void>(world.getComponents().remove<Health>(healed));
    static_cast<void>(world.getComponents().add(healed, Health{.value = 5}));  // lost it, then got it back

    EXPECT_EQ(visit(query, lastRun), (std::vector<Entity>{armored, healed}));
}

TEST(RemovedQuery, NothingIsKeptWhileNoQueryWatches) {
    World world;
    const Entity first = world.spawn(Health{.value = 1});
    static_cast<void>(world.getComponents().remove<Health>(first));
    EXPECT_EQ(keptRemovals<Health>(world), 0U);

    {
        const QueryState<Removed<Health>> query{world.getComponents()};
        const Entity second = world.spawn(Health{.value = 1});
        static_cast<void>(world.getComponents().remove<Health>(second));
        EXPECT_EQ(keptRemovals<Health>(world), 1U);
    }

    const Entity third = world.spawn(Health{.value = 1});
    static_cast<void>(world.getComponents().remove<Health>(third));
    EXPECT_EQ(keptRemovals<Health>(world), 0U);  // the query is gone: its reader too
}

TEST(RemovedQuery, ARemovalIsDroppedOnceEveryQuerySawIt) {
    World world;
    const QueryState<Removed<Health>> often{world.getComponents()};
    const QueryState<Removed<Health>> rarely{world.getComponents()};
    const Tick start = world.getChangeTick();
    world.advanceChangeTick();
    static_cast<void>(world.getComponents().remove<Health>(world.spawn(Health{.value = 1})));

    const Tick oftenRun = world.getChangeTick();
    EXPECT_EQ(visit(often, start).size(), 1U);
    world.advanceChangeTick();
    static_cast<void>(visit(often, oftenRun));  // `often` has seen the first removal
    static_cast<void>(world.getComponents().remove<Health>(world.spawn(Health{.value = 1})));
    EXPECT_EQ(keptRemovals<Health>(world), 2U);  // `rarely` has not run yet

    EXPECT_EQ(visit(rarely, start).size(), 2U);
    const Tick rarelyRun = world.getChangeTick();
    world.advanceChangeTick();
    static_cast<void>(visit(often, rarelyRun));
    static_cast<void>(visit(rarely, rarelyRun));
    static_cast<void>(world.getComponents().remove<Health>(world.spawn(Health{.value = 1})));
    EXPECT_EQ(keptRemovals<Health>(world), 1U);  // only the newest
}

TEST(RemovedQuery, ADynamicQueryVisitsWhatLostAComponentAndReadsIt) {
    World world;
    const aylo::ecs::ComponentId health = world.getComponents().registerType<Health>();
    const aylo::ecs::Result<aylo::ecs::DynamicQuery> query =
        aylo::ecs::QueryBuilder{}.removed(health).build(world.getComponents());
    ASSERT_TRUE(query);
    ASSERT_EQ(query->getAccess().size(), 1U);
    EXPECT_FALSE(query->getAccess().front().write);
    EXPECT_EQ(query->getDataTermCount(), 0U);
    const Entity entity = world.spawn(Health{.value = 1});
    const Tick lastRun = world.getChangeTick();
    world.advanceChangeTick();
    static_cast<void>(world.despawn(entity));

    std::vector<Entity> seen;
    for (aylo::ecs::QueryMatch& match : query->iter(lastRun)) {
        seen.push_back(match.getEntity());
        EXPECT_TRUE(match.get(0).empty());
    }

    EXPECT_EQ(seen, std::vector<Entity>{entity});
}

TEST(RemovedQuery, RemovalLogKeepsWhatAReaderHasNotSeen) {
    aylo::ecs::storage::RemovalLog log;
    log.record(Entity{}, 1);
    EXPECT_TRUE(log.getRemovals().empty());  // nobody watches
    const std::shared_ptr<Tick> reader = log.watch(1);
    const Entity entity = World{}.spawn();
    log.record(entity, 2);
    EXPECT_TRUE(log.contains(entity, 1, 2));
    EXPECT_FALSE(log.contains(entity, 2, 2));
    *reader = 2;
    log.record(entity, 3);
    ASSERT_EQ(log.getRemovals().size(), 1U);
    EXPECT_EQ(log.getRemovals().front().tick, 3U);
}
