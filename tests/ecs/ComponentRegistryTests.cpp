/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ComponentRegistryTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <string>
#include <utility>
#include <vector>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/exceptions/ComponentExceptions.hpp"
#include "core/hash/NameHash.hpp"

namespace aylo::test::registry {
struct Health {
    std::int32_t value;
    std::int32_t max;
};
AYLO_ECS_COMPONENT(Health, "rtype.Health", value, max);

struct Transform {
    float x;
    float y;
};
AYLO_ECS_COMPONENT(Transform, "rtype.Transform", x, y);

struct Dead {};
AYLO_ECS_COMPONENT(Dead, "rtype.Dead");

/// @brief Another struct claiming the name "rtype.Health", with another layout.
struct OtherHealth {
    float value;
};
AYLO_ECS_COMPONENT(OtherHealth, "rtype.Health", value);
}  // namespace aylo::test::registry

namespace {
/// @return A run-time description, as a script would provide it.
aylo::ecs::ComponentInfo makeShield() {
    return aylo::ecs::ComponentInfo{
        .type = {.name = "game.Shield",
                 .size = 8,
                 .alignment = 4,
                 .fields = {{.name = "strength", .type = aylo::ecs::FieldType::kF32, .offset = 0},
                            {.name = "regen", .type = aylo::ecs::FieldType::kF32, .offset = 4}}}};
}
}  // namespace

TEST(ComponentRegistry, GivesDenseIdsFromZero) {
    aylo::ecs::ComponentRegistry registry;
    EXPECT_EQ(registry.registerType<aylo::test::registry::Health>(), 0U);
    EXPECT_EQ(registry.registerType<aylo::test::registry::Transform>(), 1U);
    EXPECT_EQ(registry.registerType<aylo::test::registry::Dead>(), 2U);
    EXPECT_EQ(registry.getCount(), 3U);
}

TEST(ComponentRegistry, RegisteringTheSameComponentTwiceReturnsTheSameId) {
    aylo::ecs::ComponentRegistry registry;
    const aylo::ecs::ComponentId first = registry.registerType<aylo::test::registry::Health>();
    EXPECT_EQ(registry.registerType<aylo::test::registry::Health>(), first);
    EXPECT_EQ(registry.getCount(), 1U);
}

TEST(ComponentRegistry, FindsComponentsByIdNameHashAndType) {
    aylo::ecs::ComponentRegistry registry;
    const aylo::ecs::ComponentId id = registry.registerType<aylo::test::registry::Health>();

    const aylo::ecs::ComponentInfo* byId = registry.findById(id);
    ASSERT_NE(byId, nullptr);
    EXPECT_EQ(byId->type.name, "rtype.Health");
    EXPECT_EQ(byId->type.id, id);
    EXPECT_EQ(registry.findByName("rtype.Health"), byId);
    EXPECT_EQ(registry.findByHash(aylo::core::hash::hashName("rtype.Health")), byId);
    EXPECT_EQ(registry.find<aylo::test::registry::Health>(), byId);
}

TEST(ComponentRegistry, FindReturnsNullForAnotherLayoutUnderTheSameName) {
    aylo::ecs::ComponentRegistry registry;
    static_cast<void>(registry.registerType<aylo::test::registry::Health>());
    EXPECT_EQ(registry.find<aylo::test::registry::OtherHealth>(), nullptr);  // "rtype.Health", but not this layout
    EXPECT_NE(registry.find<aylo::test::registry::Health>(), nullptr);
}

TEST(ComponentRegistry, FindReturnsNullForARunTimeComponentThatOnlySharesTheName) {
    aylo::ecs::ComponentRegistry registry;
    aylo::ecs::ComponentInfo script = makeShield();  // 8 bytes like Health, other fields
    script.type.name = "rtype.Health";
    ASSERT_TRUE(registry.registerType(std::move(script)));
    EXPECT_EQ(registry.find<aylo::test::registry::Health>(), nullptr);
    EXPECT_THROW(static_cast<void>(registry.registerType<aylo::test::registry::Health>()),
                 aylo::ecs::exceptions::ComponentRegistrationException);
}

TEST(ComponentRegistry, FindAcceptsARunTimeDescriptionOfTheSameLayout) {
    aylo::ecs::ComponentRegistry registry;
    // A script describing exactly the C++ layout is the same component.
    ASSERT_TRUE(registry.registerType(aylo::ecs::ComponentTraits<aylo::test::registry::Health>::makeInfo()));
    EXPECT_NE(registry.find<aylo::test::registry::Health>(), nullptr);
}

TEST(ComponentRegistry, ReturnsNullForUnknownComponents) {
    aylo::ecs::ComponentRegistry registry;
    static_cast<void>(registry.registerType<aylo::test::registry::Health>());
    EXPECT_EQ(registry.findById(1), nullptr);
    EXPECT_EQ(registry.findById(aylo::ecs::kInvalidComponentId), nullptr);
    EXPECT_EQ(registry.findByName("rtype.Unknown"), nullptr);
    EXPECT_EQ(registry.findByHash(aylo::core::hash::hashName("rtype.Unknown")), nullptr);
    EXPECT_EQ(registry.find<aylo::test::registry::Transform>(), nullptr);
}

TEST(ComponentRegistry, SetsTheLayoutHash) {
    aylo::ecs::ComponentRegistry registry;
    static_cast<void>(registry.registerType<aylo::test::registry::Health>());
    const aylo::ecs::ComponentInfo* info = registry.find<aylo::test::registry::Health>();
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->type.layoutHash, aylo::ecs::computeLayoutHash(info->type));
    EXPECT_NE(info->type.layoutHash, 0U);
}

TEST(ComponentRegistry, AppliesTheOptions) {
    aylo::ecs::ComponentRegistry registry;
    static_cast<void>(registry.registerType<aylo::test::registry::Transform>(
        {.storage = aylo::ecs::StorageKind::kTable, .replication = aylo::ecs::ReplicationMode::kEveryChange}));
    const aylo::ecs::ComponentInfo* info = registry.find<aylo::test::registry::Transform>();
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->storage, aylo::ecs::StorageKind::kTable);
    EXPECT_EQ(info->type.presence, aylo::ecs::Presence::kBoth);
    EXPECT_EQ(info->type.replication, aylo::ecs::ReplicationMode::kEveryChange);
}

TEST(ComponentRegistry, ListsOnlyReplicatedComponents) {
    aylo::ecs::ComponentRegistry registry;
    static_cast<void>(registry.registerType<aylo::test::registry::Health>());
    const aylo::ecs::ComponentId transform = registry.registerType<aylo::test::registry::Transform>(
        {.replication = aylo::ecs::ReplicationMode::kEveryChange});
    const aylo::ecs::ComponentId dead =
        registry.registerType<aylo::test::registry::Dead>({.replication = aylo::ecs::ReplicationMode::kSpawnOnly});
    static_cast<void>(registry.registerType<aylo::test::registry::Transform>(
        {.replication = aylo::ecs::ReplicationMode::kEveryChange}));  // again: listed once

    const std::vector<aylo::ecs::ComponentId> replicated{registry.getReplicatedIds().begin(),
                                                         registry.getReplicatedIds().end()};
    EXPECT_EQ(replicated, (std::vector<aylo::ecs::ComponentId>{transform, dead}));
}

TEST(ComponentRegistry, RegistersRunTimeDescriptions) {
    aylo::ecs::ComponentRegistry registry;
    const aylo::ecs::Result<aylo::ecs::ComponentId> id = registry.registerType(makeShield());
    ASSERT_TRUE(id);
    EXPECT_EQ(*id, 0U);
    const aylo::ecs::Result<aylo::ecs::ComponentId> again = registry.registerType(makeShield());
    ASSERT_TRUE(again);
    EXPECT_EQ(*again, *id);
    EXPECT_NE(registry.findByName("game.Shield"), nullptr);
}

TEST(ComponentRegistry, RefusesAnInvalidDescription) {
    aylo::ecs::ComponentRegistry registry;
    aylo::ecs::ComponentInfo info = makeShield();
    info.type.alignment = 3;
    const aylo::ecs::Result<aylo::ecs::ComponentId> id = registry.registerType(std::move(info));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), aylo::ecs::ErrorKind::kInvalidType);
    EXPECT_EQ(registry.getCount(), 0U);
}

TEST(ComponentRegistry, RefusesTheSameNameWithAnotherLayout) {
    aylo::ecs::ComponentRegistry registry;
    ASSERT_TRUE(registry.registerType(makeShield()));
    aylo::ecs::ComponentInfo bigger = makeShield();
    bigger.type.size = 16;
    const aylo::ecs::Result<aylo::ecs::ComponentId> id = registry.registerType(std::move(bigger));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), aylo::ecs::ErrorKind::kLayoutMismatch);
    EXPECT_NE(std::string{id.error().getMessage()}.find("game.Shield"), std::string::npos);
    EXPECT_EQ(registry.getCount(), 1U);
}

TEST(ComponentRegistry, RefusesTheSameLayoutWithOtherHooks) {
    aylo::ecs::ComponentRegistry registry;
    ASSERT_TRUE(registry.registerType(makeShield()));
    aylo::ecs::ComponentInfo managed = makeShield();  // same name, size and fields, but needs destroying
    managed.type.hooks.destruct = [](void* /*target*/) noexcept {};
    managed.type.hooks.copy = [](void* /*destination*/, const void* /*source*/) {};  // consistent: valid on its own
    const aylo::ecs::Result<aylo::ecs::ComponentId> id = registry.registerType(std::move(managed));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), aylo::ecs::ErrorKind::kLayoutMismatch);
    EXPECT_EQ(registry.getCount(), 1U);
}

TEST(ComponentRegistry, RefusesTheSameNameWithOtherOptions) {
    aylo::ecs::ComponentRegistry registry;
    ASSERT_TRUE(registry.registerType(makeShield()));
    aylo::ecs::ComponentInfo replicated = makeShield();
    replicated.type.replication = aylo::ecs::ReplicationMode::kEveryChange;
    const aylo::ecs::Result<aylo::ecs::ComponentId> id = registry.registerType(std::move(replicated));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), aylo::ecs::ErrorKind::kOptionsMismatch);
    const std::string message{id.error().getMessage()};
    EXPECT_NE(message.find("replication 'none', not 'everyChange'"), std::string::npos) << message;
    EXPECT_TRUE(registry.getReplicatedIds().empty());
}

TEST(ComponentRegistry, TypedRegistrationThrowsOnConflicts) {
    aylo::ecs::ComponentRegistry registry;
    static_cast<void>(registry.registerType<aylo::test::registry::Health>());
    EXPECT_THROW(static_cast<void>(registry.registerType<aylo::test::registry::OtherHealth>()),
                 aylo::ecs::exceptions::ComponentRegistrationException);
    EXPECT_THROW(static_cast<void>(registry.registerType<aylo::test::registry::Health>(
                     {.replication = aylo::ecs::ReplicationMode::kEveryChange})),
                 aylo::ecs::exceptions::ComponentRegistrationException);
}

TEST(ComponentRegistry, PointersStayValidAfterManyRegistrations) {
    aylo::ecs::ComponentRegistry registry;
    const aylo::ecs::ComponentId health = registry.registerType<aylo::test::registry::Health>();
    const aylo::ecs::ComponentInfo* info = registry.findById(health);
    for (int i = 0; i < 500; ++i) {
        aylo::ecs::ComponentInfo other = makeShield();
        other.type.name = std::format("game.Shield{}", i);
        ASSERT_TRUE(registry.registerType(std::move(other)));
    }
    EXPECT_EQ(registry.findById(health), info);
    EXPECT_EQ(info->type.name, "rtype.Health");
}

TEST(ComponentRegistry, KeepsItsComponentsWhenMoved) {
    aylo::ecs::ComponentRegistry registry;
    const aylo::ecs::ComponentId health = registry.registerType<aylo::test::registry::Health>();
    const aylo::ecs::ComponentRegistry moved{std::move(registry)};
    ASSERT_NE(moved.findById(health), nullptr);
    EXPECT_EQ(moved.findByName("rtype.Health")->type.id, health);
}
