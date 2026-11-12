/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FailureTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

#include "aylo/luau/ErrorKind.hpp"
#include "aylo/luau/Failure.hpp"
#include "aylo/luau/Result.hpp"

namespace {
aylo::luau::Result<int> failWithValue() { return aylo::luau::Failure{aylo::luau::ErrorKind::kRuntime, "boom"}; }

aylo::luau::Result<> failWithVoid() { return aylo::luau::Failure{aylo::luau::ErrorKind::kSyntax, "bad chunk"}; }

aylo::luau::Result<std::unique_ptr<int>> failWithMoveOnly() {
    return aylo::luau::Failure{aylo::luau::ErrorKind::kOutOfMemory, "oom"};
}
}  // namespace

TEST(Failure, ConvertsToValueResult) {
    const auto result = failWithValue();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kRuntime);
    EXPECT_EQ(result.error().message(), "boom");
}

TEST(Failure, ConvertsToVoidResult) {
    const auto result = failWithVoid();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kSyntax);
    EXPECT_EQ(result.error().message(), "bad chunk");
}

TEST(Failure, ConvertsToMoveOnlyResult) {
    const auto result = failWithMoveOnly();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kOutOfMemory);
}

TEST(Failure, AcceptsTraceback) {
    const aylo::luau::Result<int> result = aylo::luau::Failure{aylo::luau::ErrorKind::kRuntime, "boom", "script:3"};

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kRuntime);
}

TEST(Failure, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<aylo::luau::Failure>);
    static_assert(!std::is_copy_assignable_v<aylo::luau::Failure>);
    static_assert(std::is_move_constructible_v<aylo::luau::Failure>);
    static_assert(std::is_move_assignable_v<aylo::luau::Failure>);
}
