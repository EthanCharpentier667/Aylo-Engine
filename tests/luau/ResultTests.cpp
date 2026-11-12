/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ResultTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <tl/expected.hpp>
#include <type_traits>
#include <utility>

#include "aylo/luau/Error.hpp"
#include "aylo/luau/ErrorKind.hpp"
#include "aylo/luau/Result.hpp"

namespace {
aylo::luau::Result<int> makeFailure(const aylo::luau::ErrorKind kind, std::string message) {
    return tl::unexpected{aylo::luau::Error{kind, std::move(message)}};
}
}  // namespace

TEST(Result, SuccessConvertsToTrue) {
    const aylo::luau::Result<int> result{42};

    EXPECT_TRUE(result);
    EXPECT_TRUE(result.has_value());
}

TEST(Result, FailureConvertsToFalse) {
    const auto result = makeFailure(aylo::luau::ErrorKind::kRuntime, "boom");

    EXPECT_FALSE(result);
    EXPECT_FALSE(result.has_value());
}

TEST(Result, DereferenceReturnsValue) {
    const aylo::luau::Result<int> result{42};

    EXPECT_EQ(*result, 42);
}

TEST(Result, ArrowOperatorAccessesValueMembers) {
    const aylo::luau::Result<std::string> result{"hello"};

    ASSERT_TRUE(result);
    EXPECT_EQ(result->size(), 5U);
}

TEST(Result, DereferenceAllowsMutation) {
    aylo::luau::Result<int> result{1};

    *result = 7;

    EXPECT_EQ(*result, 7);
}

TEST(Result, ErrorExposesKindAndMessage) {
    const auto result = makeFailure(aylo::luau::ErrorKind::kTimeout, "took too long");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kTimeout);
    EXPECT_EQ(result.error().message(), "took too long");
}

TEST(Result, AcceptsErrorBuiltWithTraceback) {
    const aylo::luau::Result<int> result{
        tl::unexpected{aylo::luau::Error{aylo::luau::ErrorKind::kRuntime, "boom", "script:3"}}};

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kRuntime);
}

TEST(Result, WorksWithCopyableType) {
    static_assert(std::is_copy_constructible_v<std::string>);

    const aylo::luau::Result<std::string> result{"copy me"};
    std::string copy = *result;
    copy += "!";

    EXPECT_EQ(copy, "copy me!");
    EXPECT_EQ(*result, "copy me");
}

TEST(Result, WorksWithMoveOnlyType) {
    aylo::luau::Result<std::unique_ptr<int>> result{std::make_unique<int>(9)};

    ASSERT_TRUE(result);
    EXPECT_EQ(**result, 9);

    const std::unique_ptr<int> taken = std::move(*result);
    ASSERT_NE(taken, nullptr);
    EXPECT_EQ(*taken, 9);
}

TEST(Result, MoveOnlyResultIsMovable) {
    aylo::luau::Result<std::unique_ptr<int>> source{std::make_unique<int>(3)};
    aylo::luau::Result<std::unique_ptr<int>> target{std::move(source)};

    ASSERT_TRUE(target);
    EXPECT_EQ(**target, 3);
}

TEST(Result, MoveOnlyFailureIsMovable) {
    aylo::luau::Result<std::unique_ptr<int>> source{
        tl::unexpected{aylo::luau::Error{aylo::luau::ErrorKind::kOutOfMemory, "oom"}}};
    const aylo::luau::Result<std::unique_ptr<int>> target{std::move(source)};

    ASSERT_FALSE(target);
    EXPECT_EQ(target.error().kind(), aylo::luau::ErrorKind::kOutOfMemory);
    EXPECT_EQ(target.error().message(), "oom");
}

TEST(ResultVoid, DefaultIsSuccess) {
    const aylo::luau::Result<> result{};

    EXPECT_TRUE(result);
    EXPECT_TRUE(result.has_value());
}

TEST(ResultVoid, ExplicitVoidIsSuccess) {
    const aylo::luau::Result<void> result{};

    EXPECT_TRUE(result);
}

TEST(ResultVoid, FailureConvertsToFalse) {
    const aylo::luau::Result<> result{tl::unexpected{aylo::luau::Error{aylo::luau::ErrorKind::kSyntax, "bad chunk"}}};

    EXPECT_FALSE(result);
}

TEST(ResultVoid, ErrorExposesKindAndMessage) {
    const aylo::luau::Result<> result{
        tl::unexpected{aylo::luau::Error{aylo::luau::ErrorKind::kStackOverflow, "too deep"}}};

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kStackOverflow);
    EXPECT_EQ(result.error().message(), "too deep");
}

TEST(Result, ForwardErrorKeepsKindAndMessage) {
    auto source = makeFailure(aylo::luau::ErrorKind::kCompilation, "bad syntax");

    const auto forwarded = aylo::luau::forwardError<std::string>(std::move(source));

    ASSERT_FALSE(forwarded);
    EXPECT_EQ(forwarded.error().kind(), aylo::luau::ErrorKind::kCompilation);
    EXPECT_EQ(forwarded.error().message(), "bad syntax");
}

TEST(Result, ForwardErrorToVoidResult) {
    auto source = makeFailure(aylo::luau::ErrorKind::kRuntime, "boom");

    const auto forwarded = aylo::luau::forwardError<void>(std::move(source));

    ASSERT_FALSE(forwarded);
    EXPECT_EQ(forwarded.error().kind(), aylo::luau::ErrorKind::kRuntime);
}
