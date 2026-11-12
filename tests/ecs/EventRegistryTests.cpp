/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EventRegistryTests
*/

#include <gtest/gtest.h>

#include <cstdint>

#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/event/EventInfo.hpp"
#include "aylo/ecs/event/EventRegistry.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/ecs/exceptions/TypeExceptions.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "core/hash/NameHash.hpp"

namespace aylo::test::event_registry {
struct Collision {
    std::uint32_t a;
    std::uint32_t b;
};
AYLO_ECS_EVENT(Collision, "test.registry.Collision", a, b);

struct Resized {
    std::int32_t width;
    std::int32_t height;
};
AYLO_ECS_EVENT(Resized, "test.registry.Resized", width, height);

/// @brief Another layout under Collision's name.
struct OtherCollision {
    std::uint64_t pair;
};
AYLO_ECS_EVENT(OtherCollision, "test.registry.Collision", pair);
}  // namespace aylo::test::event_registry

namespace evr = aylo::test::event_registry;

TEST(EventRegistry, GivesDenseIdsFromZero) {
    aylo::ecs::EventRegistry registry;
    EXPECT_EQ(registry.registerType<evr::Collision>(), 0U);
    EXPECT_EQ(registry.registerType<evr::Resized>(), 1U);
    EXPECT_EQ(registry.getCount(), 2U);
}

TEST(EventRegistry, RegisteringTheSameEventTwiceReturnsTheSameId) {
    aylo::ecs::EventRegistry registry;
    const aylo::ecs::EventId id = registry.registerType<evr::Collision>();
    EXPECT_EQ(registry.registerType<evr::Collision>(), id);
    EXPECT_EQ(registry.getCount(), 1U);
}

TEST(EventRegistry, FindsEventsByIdNameHashAndType) {
    aylo::ecs::EventRegistry registry;
    const aylo::ecs::EventId id = registry.registerType<evr::Collision>();
    const aylo::ecs::EventInfo* info = registry.findById(id);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(registry.findByName("test.registry.Collision"), info);
    EXPECT_EQ(registry.findByHash(aylo::core::hash::hashName("test.registry.Collision")), info);
    EXPECT_EQ(registry.find<evr::Collision>(), info);
    EXPECT_EQ(registry.find<evr::Resized>(), nullptr);
}

TEST(EventRegistry, AppliesThePresence) {
    aylo::ecs::EventRegistry registry;
    registry.registerType<evr::Resized>({.presence = aylo::ecs::Presence::kClientOnly});
    EXPECT_EQ(registry.find<evr::Resized>()->type.presence, aylo::ecs::Presence::kClientOnly);
}

TEST(EventRegistry, RefusesAnotherPresenceForTheSameName) {
    aylo::ecs::EventRegistry registry;
    registry.registerType<evr::Resized>({.presence = aylo::ecs::Presence::kClientOnly});
    EXPECT_THROW(registry.registerType<evr::Resized>({.presence = aylo::ecs::Presence::kServerOnly}),
                 aylo::ecs::exceptions::TypeRegistrationException);
}

TEST(EventRegistry, RefusesTheSameNameWithAnotherLayout) {
    aylo::ecs::EventRegistry registry;
    registry.registerType<evr::Collision>();
    EXPECT_THROW(registry.registerType<evr::OtherCollision>(), aylo::ecs::exceptions::TypeRegistrationException);
    EXPECT_EQ(registry.find<evr::OtherCollision>(), nullptr);
}

TEST(EventRegistry, RefusesAReplicatedRunTimeEvent) {
    aylo::ecs::EventRegistry registry;
    aylo::ecs::EventInfo info = aylo::ecs::EventTraits<evr::Collision>::makeInfo();
    info.type.replication = aylo::ecs::ReplicationMode::kEveryChange;
    const aylo::ecs::Result<aylo::ecs::EventId> id = registry.registerType(info);
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), aylo::ecs::ErrorKind::kInvalidType);
    EXPECT_EQ(registry.getCount(), 0U);
}
