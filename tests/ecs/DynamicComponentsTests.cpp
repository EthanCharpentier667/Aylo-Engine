/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DynamicComponentsTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>
#include <span>
#include <string_view>

#include "aylo/ecs/change/Tick.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/exceptions/ComponentExceptions.hpp"
#include "aylo/ecs/system/DynamicComponents.hpp"
#include "aylo/ecs/system/ScriptCallable.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/system/System.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::test::anyentity {
struct Health {
    int value{0};
};
AYLO_ECS_COMPONENT(Health, "test.anyentity.Health", value);
struct Armor {
    int value{0};
};
AYLO_ECS_COMPONENT(Armor, "test.anyentity.Armor", value);
}  // namespace aylo::test::anyentity

namespace {
using aylo::ecs::ComponentId;
using aylo::ecs::ComponentList;
using aylo::ecs::DynamicComponents;
using aylo::ecs::Entity;
using aylo::ecs::ScriptCallable;
using aylo::ecs::Side;
using aylo::ecs::System;
using aylo::ecs::World;
using aylo::test::anyentity::Armor;
using aylo::test::anyentity::Health;

/// @return The int at the start of `bytes`.
int intOf(std::span<const std::byte> bytes) {
    int value = 0;
    std::memcpy(&value, bytes.data(), sizeof(int));
    return value;
}
}  // namespace

TEST(DynamicComponents, ADeclaredComponentIsReadOnAnyEntity) {
    World world;
    const Entity target = world.spawn(Health{.value = 7});
    const Entity other = world.spawn();
    const ComponentId health = world.getComponents().registerType<Health>();
    int seen = 0;
    bool otherHas = true;
    System system{world, "test.read", ScriptCallable{[&](const DynamicComponents components) {
                      seen = intOf(components.get(target, 0));
                      otherHas = components.has(other, 0);
                      EXPECT_TRUE(components.getMut(target, 0).empty());  // declared read
                  }},
                  ComponentList{{{.id = health, .write = false}}}};
    ASSERT_EQ(system.getDeclaredAccess().getComponents().size(), 1U);

    system.run();

    EXPECT_EQ(seen, 7);
    EXPECT_FALSE(otherHas);
}

TEST(DynamicComponents, AComponentFoundWhileRunningIsReadThenWritten) {
    World world;
    const Entity target = world.spawn(Health{.value = 7});
    const ComponentId health = world.getComponents().registerType<Health>();
    System system{world, "test.write", ScriptCallable{[&](const DynamicComponents components) {
                      const std::size_t index = components.add(health, false);
                      EXPECT_TRUE(components.getMut(target, index).empty());  // read, so far
                      EXPECT_EQ(components.add(health, true), index);
                      const std::span<std::byte> bytes = components.getMut(target, index);
                      const int value = intOf(bytes) + 1;
                      std::memcpy(bytes.data(), &value, sizeof(int));
                  }},
                  ComponentList{}};
    EXPECT_TRUE(system.getAccess().getComponents().empty());
    world.advanceChangeTick();
    const aylo::ecs::Tick spawned = world.getComponents().getTicks(target, health)->changed;

    system.run();

    EXPECT_EQ(world.getComponents().get<Health>(target)->value, 8);
    ASSERT_EQ(system.getAccess().getComponents().size(), 1U);
    EXPECT_TRUE(system.getAccess().getComponents().front().write);
    EXPECT_TRUE(system.getDeclaredAccess().getComponents().empty());
    EXPECT_TRUE(system.takeAccessGrowth());
    EXPECT_NE(world.getComponents().getTicks(target, health)->changed, spawned);  // marked changed by the write
}

TEST(DynamicComponents, ADeadEntityHasNothing) {
    World world;
    const Entity target = world.spawn(Health{.value = 7});
    const ComponentId health = world.getComponents().registerType<Health>();
    world.despawn(target);
    System system{world, "test.dead", ScriptCallable{[&](const DynamicComponents components) {
                      EXPECT_FALSE(components.has(target, 0));
                      EXPECT_TRUE(components.get(target, 0).empty());
                      EXPECT_TRUE(components.getMut(target, 0).empty());
                  }},
                  ComponentList{{{.id = health, .write = true}}}};
    system.run();
}

TEST(DynamicComponents, AComponentCannotBeAddedOutsideASystemRun) {
    World world;
    const ComponentId health = world.getComponents().registerType<Health>();
    ComponentList list;
    const DynamicComponents components{list, {.world = &world}};
    EXPECT_THROW(static_cast<void>(components.add(health, false)),
                 aylo::ecs::exceptions::ComponentRegistrationException);
    EXPECT_THROW(static_cast<void>(components.add(health + 1000, false)),
                 aylo::ecs::exceptions::ComponentRegistrationException);
}

TEST(DynamicComponents, AClientSystemReadsButCannotWriteAReplicatedComponent) {
    World world;
    const Entity target = world.spawn();
    const ComponentId health =
        world.getComponents().registerType<Health>({.replication = aylo::ecs::ReplicationMode::kEveryChange});
    static_cast<void>(world.getComponents().add(target, Health{.value = 3}));
    bool refused = false;
    System system{world, "test.client", ScriptCallable{[&](const DynamicComponents components) {
                      const std::size_t index = components.add(health, false);
                      EXPECT_EQ(intOf(components.get(target, index)), 3);
                      try {
                          static_cast<void>(components.add(health, true));
                      } catch (const aylo::ecs::exceptions::ComponentRegistrationException& error) {
                          refused = std::string_view{error.what()}.find("replicated") != std::string_view::npos;
                      }
                  }},
                  ComponentList{}};
    system.setSide(Side::kClient);

    system.run();

    EXPECT_TRUE(refused);
    ASSERT_EQ(system.getAccess().getComponents().size(), 1U);
    EXPECT_FALSE(system.getAccess().getComponents().front().write);
}

TEST(DynamicComponents, AComponentIsReadAsItsCxxType) {
    World world;
    const Entity target = world.spawn(Health{.value = 7});
    const ComponentId health = world.getComponents().registerType<Health>();
    const ComponentId other = world.getComponents().registerType<Armor>();
    System system{world, "test.typed", ScriptCallable{[&](const DynamicComponents components) {
                      const Health* value = components.get<Health>(target, 0);
                      ASSERT_NE(value, nullptr);
                      EXPECT_EQ(value->value, 7);
                      EXPECT_EQ(components.get<Health>(target, 1), nullptr);  // index 1 is Armor, not Health
                      EXPECT_EQ(components.get<Armor>(target, 1), nullptr);   // the entity has no Armor
                  }},
                  ComponentList{{{.id = health, .write = false}, {.id = other, .write = false}}}};
    system.run();
}
