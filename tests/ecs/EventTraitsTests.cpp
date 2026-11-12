/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EventTraitsTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/event/EventInfo.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/type/TypeTraits.hpp"
#include "core/hash/NameHash.hpp"

namespace aylo::test::event_traits {
struct Collision {
    std::uint32_t a;
    std::uint32_t b;
};
AYLO_ECS_EVENT(Collision, "test.traits.Collision", a, b);

/// @brief A move-only event, like one carrying a buffer it owns.
struct Packet {
    std::unique_ptr<std::int32_t> payload;
};
AYLO_ECS_EVENT(Packet, "test.traits.Packet");

struct GameOver {};
AYLO_ECS_EVENT(GameOver, "test.traits.GameOver");

struct Health {
    std::int32_t value;
};
AYLO_ECS_COMPONENT(Health, "test.traits.Health", value);

struct Time {
    float delta;
};
AYLO_ECS_RESOURCE(Time, "test.traits.Time");
}  // namespace aylo::test::event_traits

namespace evt = aylo::test::event_traits;

TEST(EventTraits, ConceptOnlyAcceptsEvents) {
    static_assert(aylo::ecs::Event<evt::Collision>);
    static_assert(aylo::ecs::Event<evt::Packet>);
    static_assert(aylo::ecs::Event<evt::GameOver>);
    static_assert(!aylo::ecs::Event<evt::Health>);
    static_assert(!aylo::ecs::Event<evt::Time>);
    static_assert(!aylo::ecs::Event<std::int32_t>);
    static_assert(!aylo::ecs::Component<evt::Collision>);
    static_assert(!aylo::ecs::Resource<evt::Collision>);
}

TEST(EventTraits, RecordsTheNameKindAndHash) {
    static_assert(aylo::ecs::EventTraits<evt::Collision>::kName == "test.traits.Collision");
    static_assert(aylo::ecs::EventTraits<evt::Collision>::kKind == aylo::ecs::TypeKind::kEvent);
    static_assert(aylo::ecs::EventTraits<evt::Collision>::kHash == aylo::core::hash::hashName("test.traits.Collision"));
}

TEST(EventTraits, MakesTheDescription) {
    const aylo::ecs::EventInfo info = aylo::ecs::EventTraits<evt::Collision>::makeInfo();
    EXPECT_EQ(info.type.id, aylo::ecs::kInvalidEventId);
    EXPECT_EQ(info.type.name, "test.traits.Collision");
    EXPECT_EQ(info.type.size, sizeof(evt::Collision));
    EXPECT_EQ(info.type.alignment, alignof(evt::Collision));
    EXPECT_TRUE(info.type.copyable);
    ASSERT_EQ(info.type.fields.size(), 2U);
    EXPECT_EQ(info.type.fields[0].name, "a");
    EXPECT_EQ(info.type.fields[1].name, "b");
}

TEST(EventTraits, AcceptsMoveOnlyEvents) {
    const aylo::ecs::EventInfo info = aylo::ecs::EventTraits<evt::Packet>::makeInfo();
    EXPECT_FALSE(info.type.copyable);
    EXPECT_NE(info.type.hooks.move, nullptr);
    EXPECT_NE(info.type.hooks.destruct, nullptr);
    EXPECT_EQ(info.type.hooks.copy, nullptr);
}

TEST(EventTraits, AnEventWithoutDataHasSizeZero) {
    EXPECT_EQ(aylo::ecs::EventTraits<evt::GameOver>::makeInfo().type.size, 0U);
}
