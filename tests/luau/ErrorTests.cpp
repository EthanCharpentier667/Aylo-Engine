/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ErrorTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <exception>
#include <source_location>
#include <string>
#include <type_traits>
#include <utility>

#include "aylo/luau/Error.hpp"
#include "aylo/luau/ErrorKind.hpp"

TEST(Error, StoresKindAndMessage) {
    const aylo::luau::Error error{aylo::luau::ErrorKind::kSyntax, "unexpected symbol"};

    EXPECT_EQ(error.kind(), aylo::luau::ErrorKind::kSyntax);
    EXPECT_EQ(error.message(), "unexpected symbol");
}

TEST(Error, StoresEveryKind) {
    constexpr std::array kinds{
        aylo::luau::ErrorKind::kRuntime,       aylo::luau::ErrorKind::kSyntax,
        aylo::luau::ErrorKind::kTypeMismatch,  aylo::luau::ErrorKind::kTimeout,
        aylo::luau::ErrorKind::kOutOfMemory,   aylo::luau::ErrorKind::kStackOverflow,
        aylo::luau::ErrorKind::kLimitExceeded, aylo::luau::ErrorKind::kUnknown,
    };

    for (const auto kind : kinds) {
        const aylo::luau::Error error{kind, "message"};
        EXPECT_EQ(error.kind(), kind);
    }
}

TEST(Error, KindsAreDistinct) {
    EXPECT_NE(aylo::luau::ErrorKind::kRuntime, aylo::luau::ErrorKind::kSyntax);
    EXPECT_NE(aylo::luau::ErrorKind::kTimeout, aylo::luau::ErrorKind::kOutOfMemory);
    EXPECT_NE(aylo::luau::ErrorKind::kStackOverflow, aylo::luau::ErrorKind::kUnknown);
}

TEST(Error, AcceptsEmptyMessage) {
    const aylo::luau::Error error{aylo::luau::ErrorKind::kUnknown, ""};

    EXPECT_TRUE(error.message().empty());
    EXPECT_STREQ(error.what(), "");
}

TEST(Error, WhatMatchesMessage) {
    const aylo::luau::Error error{aylo::luau::ErrorKind::kRuntime, "boom"};

    EXPECT_STREQ(error.what(), "boom");
    EXPECT_EQ(error.message(), error.what());
}

TEST(Error, WhatIsNullTerminatedAndStable) {
    const aylo::luau::Error error{aylo::luau::ErrorKind::kRuntime, "boom"};

    EXPECT_EQ(error.what(), error.what());
    EXPECT_EQ(std::strlen(error.what()), error.message().size());
}

TEST(Error, IsAStdException) {
    static_assert(std::is_base_of_v<std::exception, aylo::luau::Error>);

    const aylo::luau::Error error{aylo::luau::ErrorKind::kRuntime, "boom"};
    const std::exception& base = error;
    EXPECT_STREQ(base.what(), "boom");
}

TEST(Error, AcceptsTraceback) {
    const aylo::luau::Error error{aylo::luau::ErrorKind::kRuntime, "boom", "script:1\nscript:2"};

    EXPECT_EQ(error.kind(), aylo::luau::ErrorKind::kRuntime);
    EXPECT_EQ(error.message(), "boom");
}

TEST(Error, AcceptsExplicitSourceLocation) {
    const aylo::luau::Error error{aylo::luau::ErrorKind::kTimeout, "too slow", std::source_location::current()};

    EXPECT_EQ(error.kind(), aylo::luau::ErrorKind::kTimeout);
    EXPECT_EQ(error.message(), "too slow");
}

TEST(Error, AcceptsTracebackAndSourceLocation) {
    const aylo::luau::Error error{aylo::luau::ErrorKind::kStackOverflow, "deep", "trace",
                                  std::source_location::current()};

    EXPECT_EQ(error.kind(), aylo::luau::ErrorKind::kStackOverflow);
    EXPECT_EQ(error.message(), "deep");
}

TEST(Error, KeepsLongMessage) {
    const std::string message(4096, 'x');
    const aylo::luau::Error error{aylo::luau::ErrorKind::kOutOfMemory, message};

    EXPECT_EQ(error.message(), message);
    EXPECT_STREQ(error.what(), message.c_str());
}

TEST(Error, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<aylo::luau::Error>);
    static_assert(!std::is_copy_assignable_v<aylo::luau::Error>);
    static_assert(std::is_nothrow_move_constructible_v<aylo::luau::Error>);
    static_assert(std::is_nothrow_move_assignable_v<aylo::luau::Error>);
}

TEST(Error, MoveConstructionKeepsState) {
    aylo::luau::Error original{aylo::luau::ErrorKind::kTypeMismatch, "expected number"};
    const aylo::luau::Error moved{std::move(original)};

    EXPECT_EQ(moved.kind(), aylo::luau::ErrorKind::kTypeMismatch);
    EXPECT_EQ(moved.message(), "expected number");
}

TEST(Error, MoveAssignmentKeepsState) {
    aylo::luau::Error source{aylo::luau::ErrorKind::kSyntax, "bad token"};
    aylo::luau::Error target{aylo::luau::ErrorKind::kRuntime, "other"};

    target = std::move(source);

    EXPECT_EQ(target.kind(), aylo::luau::ErrorKind::kSyntax);
    EXPECT_EQ(target.message(), "bad token");
}
