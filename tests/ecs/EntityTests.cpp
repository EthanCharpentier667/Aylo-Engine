/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EntityTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>

#include "aylo/ecs/detail/Generation.hpp"
#include "aylo/ecs/entity/Entity.hpp"

static_assert(sizeof(aylo::ecs::Entity) == 8);
static_assert(std::is_trivially_copyable_v<aylo::ecs::Entity>);

TEST(Entity, DefaultIsNull) {
    const aylo::ecs::Entity entity{};
    EXPECT_TRUE(entity.isNull());
    EXPECT_EQ(entity.getIndex(), 0U);
    EXPECT_EQ(entity.getGeneration(), 0U);
    EXPECT_EQ(entity.getBits(), 0U);
}

TEST(Entity, KeepsIndexAndGeneration) {
    const aylo::ecs::Entity entity{42, 7};
    EXPECT_FALSE(entity.isNull());
    EXPECT_EQ(entity.getIndex(), 42U);
    EXPECT_EQ(entity.getGeneration(), 7U);
}

TEST(Entity, KeepsMaximumValues) {
    constexpr std::uint32_t kMax = std::numeric_limits<std::uint32_t>::max();
    const aylo::ecs::Entity entity{kMax, kMax};
    EXPECT_EQ(entity.getIndex(), kMax);
    EXPECT_EQ(entity.getGeneration(), kMax);
}

TEST(Entity, ComparesIndexAndGeneration) {
    EXPECT_EQ((aylo::ecs::Entity{3, 1}), (aylo::ecs::Entity{3, 1}));
    EXPECT_NE((aylo::ecs::Entity{3, 1}), (aylo::ecs::Entity{3, 2}));
    EXPECT_NE((aylo::ecs::Entity{3, 1}), (aylo::ecs::Entity{4, 1}));
}

TEST(Entity, HashDiffersAcrossGenerations) {
    const std::hash<aylo::ecs::Entity> hash{};
    EXPECT_EQ(hash(aylo::ecs::Entity{3, 1}), hash(aylo::ecs::Entity{3, 1}));
    EXPECT_NE(hash(aylo::ecs::Entity{3, 1}), hash(aylo::ecs::Entity{3, 2}));
}

TEST(Generation, IncrementsBelowMaximum) {
    EXPECT_EQ(aylo::ecs::detail::nextGeneration(1), 2U);
    EXPECT_EQ(aylo::ecs::detail::nextGeneration(std::numeric_limits<std::uint32_t>::max() - 1),
              std::numeric_limits<std::uint32_t>::max());
}

TEST(Generation, IsEmptyAtMaximum) {
    EXPECT_FALSE(aylo::ecs::detail::nextGeneration(std::numeric_limits<std::uint32_t>::max()));
}
