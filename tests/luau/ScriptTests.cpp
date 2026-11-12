/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptTests
*/

#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

#include "aylo/luau/Runtime.hpp"
#include "aylo/luau/RuntimeConfig.hpp"
#include "aylo/luau/Script.hpp"

namespace {
aylo::luau::Runtime makeRuntime() {
    auto runtime = aylo::luau::Runtime::create({
        .libs = aylo::luau::RuntimeConfig::Libs::kStandard,
        .optimizationLevel = aylo::luau::RuntimeConfig::OptimizationLevel::kStandard,
    });
    EXPECT_TRUE(runtime);
    return std::move(*runtime);
}
}  // namespace

TEST(Script, ExposesName) {
    const auto runtime = makeRuntime();

    const auto script = runtime.load("main.luau", "local x = 1");

    ASSERT_TRUE(script);
    EXPECT_EQ(script->name(), "main.luau");
    EXPECT_NE(script->state(), nullptr);
}

TEST(Script, MoveConstructionTransfersParts) {
    const auto runtime = makeRuntime();
    auto source = runtime.load("moved.luau", "assert(1 + 1 == 2)");
    ASSERT_TRUE(source);

    const aylo::luau::Script moved{std::move(*source)};

    EXPECT_EQ(moved.name(), "moved.luau");
    EXPECT_TRUE(runtime.run(moved));
}

TEST(Script, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<aylo::luau::Script>);
    static_assert(!std::is_copy_assignable_v<aylo::luau::Script>);
    static_assert(std::is_nothrow_move_constructible_v<aylo::luau::Script>);
    static_assert(std::is_nothrow_move_assignable_v<aylo::luau::Script>);
    SUCCEED();
}
