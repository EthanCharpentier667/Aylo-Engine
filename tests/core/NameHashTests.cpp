/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** NameHashTests
*/

#include <gtest/gtest.h>

#include <cstdint>

#include "core/hash/NameHash.hpp"

static_assert(aylo::core::hash::hashName("rtype.Health") != aylo::core::hash::hashName("rtype.Position"));

TEST(NameHash, MatchesTheFnv1aReferenceValues) {
    EXPECT_EQ(aylo::core::hash::hashName(""), 0xcbf29ce484222325ULL);
    EXPECT_EQ(aylo::core::hash::hashName("a"), 0xaf63dc4c8601ec8cULL);
}

TEST(NameHash, DependsOnEveryCharacter) {
    EXPECT_NE(aylo::core::hash::hashName("rtype.Health"), aylo::core::hash::hashName("rtype.health"));
    EXPECT_NE(aylo::core::hash::hashName("ab"), aylo::core::hash::hashName("ba"));
    EXPECT_EQ(aylo::core::hash::hashName("rtype.Health"), aylo::core::hash::hashName("rtype.Health"));
}

TEST(NameHash, IsTheStringStepFromTheOffsetBasis) {
    EXPECT_EQ(aylo::core::hash::hashName("rtype.Health"),
              aylo::core::hash::hashString(aylo::core::hash::kNameHashOffsetBasis, "rtype.Health"));
}

TEST(NameHash, IntegerStepDependsOnEveryByte) {
    const std::uint64_t start = aylo::core::hash::kNameHashOffsetBasis;
    EXPECT_NE(aylo::core::hash::hashInteger(start, 1), aylo::core::hash::hashInteger(start, 2));
    EXPECT_NE(aylo::core::hash::hashInteger(start, 0x0100000000000000ULL), aylo::core::hash::hashInteger(start, 0));
    EXPECT_EQ(aylo::core::hash::hashInteger(start, 42), aylo::core::hash::hashInteger(start, 42));
}
