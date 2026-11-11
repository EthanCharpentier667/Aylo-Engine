/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AssetIdTests
*/

#include <gtest/gtest.h>

#include <functional>

#include "core/asset/AssetId.hpp"

static_assert(sizeof(aylo::core::asset::AssetId) == 8);

TEST(AssetId, DefaultsToNoAsset) { EXPECT_EQ(aylo::core::asset::AssetId{}.value, 0U); }

TEST(AssetId, IsTheSameForTheSamePath) {
    EXPECT_EQ(aylo::core::asset::AssetId::fromPath("assets/ship.png"),
              aylo::core::asset::AssetId::fromPath("assets/ship.png"));
    EXPECT_NE(aylo::core::asset::AssetId::fromPath("assets/ship.png").value, 0U);
}

TEST(AssetId, DiffersBetweenPaths) {
    EXPECT_NE(aylo::core::asset::AssetId::fromPath("assets/ship.png"),
              aylo::core::asset::AssetId::fromPath("assets/enemy.png"));
}

TEST(AssetId, NormalizesThePath) {
    EXPECT_EQ(aylo::core::asset::AssetId::fromPath("assets/./sprites/../ship.png"),
              aylo::core::asset::AssetId::fromPath("assets/ship.png"));
}

TEST(AssetId, HashesAsItsValue) {
    const aylo::core::asset::AssetId id = aylo::core::asset::AssetId::fromPath("assets/ship.png");
    EXPECT_EQ(std::hash<aylo::core::asset::AssetId>{}(id), static_cast<std::size_t>(id.value));
}
