/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ErrorTests
*/

#include <gtest/gtest.h>

#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"

namespace {
aylo::ecs::Result<int> parsePositive(int value) {
    if (value < 0) {
        return aylo::ecs::fail(aylo::ecs::ErrorKind::kInvalidType, "negative");
    }
    return value;
}

aylo::ecs::Result<void> check(bool ok) {
    if (!ok) {
        return aylo::ecs::fail(aylo::ecs::ErrorKind::kInvalidType, "not ok");
    }
    return {};
}
}  // namespace

TEST(Error, KeepsKindAndMessage) {
    const aylo::ecs::Error error{aylo::ecs::ErrorKind::kInvalidType, "broken"};
    EXPECT_EQ(error.getKind(), aylo::ecs::ErrorKind::kInvalidType);
    EXPECT_EQ(error.getMessage(), "broken");
}

TEST(Error, IsCopyable) {
    const aylo::ecs::Error error{aylo::ecs::ErrorKind::kInvalidType, "broken"};
    const aylo::ecs::Error copy{error};  // NOLINT(performance-unnecessary-copy-initialization)
    EXPECT_EQ(copy.getMessage(), error.getMessage());
}

TEST(Result, HoldsAValueOnSuccess) {
    const aylo::ecs::Result<int> result = parsePositive(3);
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 3);
    EXPECT_TRUE(check(true));
}

TEST(Result, FailBuildsTheErrorSide) {
    const aylo::ecs::Result<int> value = parsePositive(-1);
    ASSERT_FALSE(value);
    EXPECT_EQ(value.error().getKind(), aylo::ecs::ErrorKind::kInvalidType);
    EXPECT_EQ(value.error().getMessage(), "negative");

    const aylo::ecs::Result<void> none = check(false);
    ASSERT_FALSE(none);
    EXPECT_EQ(none.error().getMessage(), "not ok");
}
