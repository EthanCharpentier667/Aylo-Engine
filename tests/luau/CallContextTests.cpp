/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CallContextTests
*/

#include <gtest/gtest.h>

#include <string>
#include <utility>

#include "aylo/luau/CallContext.hpp"
#include "aylo/luau/ErrorKind.hpp"
#include "aylo/luau/Function.hpp"
#include "aylo/luau/Ref.hpp"
#include "aylo/luau/Runtime.hpp"
#include "aylo/luau/RuntimeConfig.hpp"
#include "aylo/luau/Value.hpp"
#include "aylo/luau/ValuePack.hpp"
#include "aylo/luau/detail/RawState.hpp"
#include "aylo/luau/detail/Stack.hpp"

namespace {
using CallContext = aylo::luau::CallContext;
using ValuePack = aylo::luau::ValuePack;

/// @brief Creates a runtime with the standard libraries, failing the test if it cannot be created.
aylo::luau::Runtime makeRuntime() {
    auto runtime = aylo::luau::Runtime::create({
        .libs = aylo::luau::RuntimeConfig::Libs::kStandard,
        .optimizationLevel = aylo::luau::RuntimeConfig::OptimizationLevel::kStandard,
    });
    EXPECT_TRUE(runtime);
    return std::move(*runtime);
}
}  // namespace

TEST(CallContext, IsEmptyByDefault) {
    const CallContext context{};

    EXPECT_EQ(context.argCount(), 0U);
    EXPECT_TRUE(context.args().empty());
}

TEST(CallContext, KeepsArgumentsInOrder) {
    ValuePack args{};
    args.add({1.0});
    args.add({std::string{"two"}});

    const CallContext context{std::move(args)};

    ASSERT_EQ(context.argCount(), 2U);
    EXPECT_EQ(context.args()[0].as<double>(), 1.0);
    EXPECT_EQ(context.args()[1].as<std::string>(), "two");
}

TEST(CallContext, MoveKeepsArguments) {
    ValuePack args{};
    args.add({true});
    CallContext context{std::move(args)};

    const CallContext moved{std::move(context)};

    EXPECT_EQ(moved.argCount(), 1U);
}

TEST(CallContext, FromStackReadsWholeStackInOrder) {
    const auto runtime = makeRuntime();
    ValuePack pushed{};
    pushed.add({std::string{"first"}});
    pushed.add({2.0});
    pushed.add({true});
    ASSERT_EQ(aylo::luau::detail::pushValuePack(aylo::luau::detail::rawState(runtime), pushed).value(), 3U);

    const auto context = aylo::luau::detail::readCallContext(aylo::luau::detail::rawState(runtime));

    ASSERT_TRUE(context);
    ASSERT_EQ(context->argCount(), 3U);
    EXPECT_EQ(context->args()[0].as<std::string>(), "first");
    EXPECT_EQ(context->args()[1].as<double>(), 2.0);
    EXPECT_EQ(context->args()[2].as<bool>(), true);
}

TEST(CallContext, FromStackOfEmptyStackHasNoArguments) {
    const auto runtime = makeRuntime();

    const auto context = aylo::luau::detail::readCallContext(aylo::luau::detail::rawState(runtime));

    ASSERT_TRUE(context);
    EXPECT_EQ(context->argCount(), 0U);
}

TEST(CallContext, FromStackFailsForUnreadableArgument) {
    const auto runtime = makeRuntime();
    auto native = aylo::luau::Function::create(runtime, "native");
    bool handlerRan{false};
    native.setHandler([&handlerRan](const CallContext&) -> ValuePack {
        handlerRan = true;
        return {};
    });
    // A thread cannot be read yet: the native closure builds its CallContext from the stack and fails.
    const auto script = runtime.load("chunk", "return native(coroutine.create(function() end))");
    ASSERT_TRUE(script);
    script->closure().push(aylo::luau::detail::rawState(runtime));
    const auto caller =
        aylo::luau::Function::fromRef(aylo::luau::Ref::pop(aylo::luau::detail::rawState(runtime)), "chunk");

    const auto result = caller.call();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kRuntime);
    EXPECT_FALSE(handlerRan);
}
