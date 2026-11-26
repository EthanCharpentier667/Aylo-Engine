/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EnabledQueryTests
*/

#include <gtest/gtest.h>

#include <string>
#include <tuple>
#include <vector>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/hierarchy/Disabled.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/query/DynamicQuery.hpp"
#include "aylo/ecs/query/QueryBuilder.hpp"
#include "aylo/ecs/query/QueryState.hpp"
#include "aylo/ecs/query/QueryTerms.hpp"
#include "aylo/ecs/query/TermTraits.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::test::enabled {
struct Health {
    int value{0};
};
AYLO_ECS_COMPONENT(Health, "test.enabled.Health", value);
}  // namespace aylo::test::enabled

namespace {
using aylo::ecs::Disabled;
using aylo::ecs::Enabled;
using aylo::ecs::Entity;
using aylo::ecs::QueryState;
using aylo::ecs::World;
using aylo::test::enabled::Health;

static_assert(!aylo::ecs::ValidQuery<const Health, Enabled, aylo::ecs::With<Disabled>>,
              "naming Disabled and skipping it contradict each other");
static_assert(!aylo::ecs::ValidQuery<Enabled>, "Enabled alone matches nothing: it is a filter");

/// @return The entities a pass of `query` visits.
template <typename Query>
std::vector<Entity> visit(const Query& query) {
    std::vector<Entity> entities;
    for (const auto& item : query.iter(0)) {
        entities.push_back(std::get<0>(item));
    }
    return entities;
}
}  // namespace

TEST(EnabledQuery, SkipsADisabledEntityAndItsDescendants) {
    World world;
    const QueryState<const Health, Enabled> enabled{world.getComponents()};
    const QueryState<const Health> all{world.getComponents()};
    const Entity root = world.spawn(Health{.value = 1});
    const Entity child = world.spawn(Health{.value = 2});
    const Entity grandchild = world.spawn(Health{.value = 3});
    const Entity other = world.spawn(Health{.value = 4});
    ASSERT_TRUE(world.setParent(child, root));
    ASSERT_TRUE(world.setParent(grandchild, child));
    EXPECT_EQ(visit(enabled).size(), 4U);  // nothing is switched off yet

    static_cast<void>(world.getComponents().add(root, Disabled{}));

    EXPECT_EQ(visit(enabled), std::vector<Entity>{other});
    EXPECT_EQ(visit(all).size(), 4U);  // a query without Enabled sees them all
    EXPECT_FALSE(enabled.get(grandchild, 0));
    EXPECT_TRUE(enabled.get(other, 0));

    static_cast<void>(world.getComponents().remove<Disabled>(root));
    EXPECT_EQ(visit(enabled).size(), 4U);
}

TEST(EnabledQuery, DisabledAndParentAreRegisteredWithTheirOwnOptions) {
    World world;
    const aylo::ecs::ComponentId disabled = world.getComponents().registerType<Disabled>();
    const aylo::ecs::ComponentId parent = world.getComponents().registerType<aylo::ecs::Parent>();
    const aylo::ecs::ComponentRegistry& registry = world.getComponents().getRegistry();
    EXPECT_EQ(registry.findById(disabled)->type.replication, aylo::ecs::ReplicationMode::kEveryChange);
    EXPECT_EQ(registry.findById(parent)->type.replication, aylo::ecs::ReplicationMode::kEveryChange);
}

TEST(EnabledQuery, ADynamicQueryTakesOnlyTheIdOfDisabled) {
    World world;
    aylo::ecs::Components& components = world.getComponents();
    const aylo::ecs::ComponentId health = components.registerType<Health>();
    const aylo::ecs::ComponentId disabled = components.registerType<Disabled>();

    const aylo::ecs::Result<aylo::ecs::DynamicQuery> good =
        aylo::ecs::QueryBuilder{}.read(health).enabled(disabled).build(components);
    const aylo::ecs::Result<aylo::ecs::DynamicQuery> wrong =
        aylo::ecs::QueryBuilder{}.read(health).enabled(health).build(components);

    ASSERT_TRUE(good);
    ASSERT_EQ(good->getAccess().size(), 2U);  // reads Health, and Disabled to skip it
    EXPECT_FALSE(good->getAccess()[1].write);
    ASSERT_FALSE(wrong);
    EXPECT_NE(wrong.error().getMessage().find("enabled() takes the id of 'aylo.Disabled'"), std::string::npos);
}
