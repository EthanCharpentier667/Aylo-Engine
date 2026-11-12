/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EventRangeTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <span>
#include <vector>

#include "aylo/ecs/event/EventCursor.hpp"
#include "aylo/ecs/event/EventRange.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/ecs/storage/ViewAs.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::test::event_range {
struct Collision {
    std::uint32_t a;
    std::uint32_t b;
};
AYLO_ECS_EVENT(Collision, "test.range.Collision", a, b);
}  // namespace aylo::test::event_range

namespace evg = aylo::test::event_range;

static_assert(std::forward_iterator<aylo::ecs::RawEventRange::Iterator>);
static_assert(std::forward_iterator<aylo::ecs::EventRange<evg::Collision>::Iterator>);
static_assert(std::ranges::forward_range<aylo::ecs::RawEventRange>);
static_assert(std::ranges::forward_range<aylo::ecs::EventRange<evg::Collision>>);

namespace {
/// @brief A world with three collisions sent, (0, 10), (1, 11) and (2, 12).
aylo::ecs::World makeWorld() {
    aylo::ecs::World world;
    for (std::uint32_t index = 0; index < 3; ++index) {
        world.getEvents().send(evg::Collision{.a = index, .b = index + 10});
    }
    return world;
}
}  // namespace

TEST(EventRange, PostIncrementReturnsThePreviousPosition) {
    const aylo::ecs::World world = makeWorld();
    aylo::ecs::EventCursor cursor;
    const aylo::ecs::EventRange<evg::Collision> collisions = world.getEvents().read<evg::Collision>(cursor);
    auto it = collisions.begin();
    const auto before = it++;
    EXPECT_EQ(before->a, 0U);
    EXPECT_EQ(it->a, 1U);
}

TEST(EventRange, WorksWithStandardAlgorithms) {
    const aylo::ecs::World world = makeWorld();
    aylo::ecs::EventCursor cursor;
    const aylo::ecs::EventRange<evg::Collision> collisions = world.getEvents().read<evg::Collision>(cursor);
    EXPECT_EQ(std::ranges::distance(collisions), 3);
    EXPECT_TRUE(std::ranges::any_of(collisions, [](const evg::Collision& collision) { return collision.b == 12U; }));
    std::vector<std::uint32_t> firsts;
    std::ranges::transform(collisions, std::back_inserter(firsts),
                           [](const evg::Collision& collision) { return collision.a; });
    EXPECT_EQ(firsts, (std::vector<std::uint32_t>{0, 1, 2}));
}

TEST(EventRange, CanBeWalkedSeveralTimes) {
    const aylo::ecs::World world = makeWorld();
    aylo::ecs::EventCursor cursor;
    const aylo::ecs::EventRange<evg::Collision> collisions = world.getEvents().read<evg::Collision>(cursor);
    EXPECT_EQ(std::ranges::distance(collisions), 3);
    EXPECT_EQ(std::ranges::distance(collisions), 3);
}

TEST(EventRange, RawRangeWorksWithStandardAlgorithms) {
    const aylo::ecs::World world = makeWorld();
    const aylo::ecs::EventId id = *world.getEvents().find("test.range.Collision");
    aylo::ecs::EventCursor cursor;
    const aylo::ecs::RawEventRange events = world.getEvents().read(id, cursor);
    EXPECT_EQ(std::ranges::distance(events), 3);
    EXPECT_TRUE(std::ranges::all_of(
        events, [](std::span<const std::byte> bytes) { return bytes.size() == sizeof(evg::Collision); }));
    auto it = events.begin();
    it++;
    EXPECT_EQ(aylo::ecs::storage::viewAs<evg::Collision>(*it)->a, 1U);
}

TEST(EventRange, DefaultIteratorsAreEqual) {
    EXPECT_EQ(aylo::ecs::RawEventRange::Iterator{}, aylo::ecs::RawEventRange::Iterator{});
    EXPECT_EQ(aylo::ecs::EventRange<evg::Collision>::Iterator{}, aylo::ecs::EventRange<evg::Collision>::Iterator{});
}
