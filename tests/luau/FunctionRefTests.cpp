/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FunctionRefTests
*/

#include <gtest/gtest.h>

#include <string>
#include <utility>

#include "aylo/luau/ErrorKind.hpp"
#include "aylo/luau/Exceptions.hpp"
#include "aylo/luau/Function.hpp"
#include "aylo/luau/FunctionRef.hpp"
#include "aylo/luau/Ref.hpp"
#include "aylo/luau/Runtime.hpp"
#include "aylo/luau/RuntimeConfig.hpp"
#include "aylo/luau/Value.hpp"
#include "aylo/luau/ValuePack.hpp"
#include "aylo/luau/detail/RawState.hpp"
#include "aylo/luau/detail/Stack.hpp"

namespace {
using ErrorKind = aylo::luau::ErrorKind;
using Function = aylo::luau::Function;
using FunctionRef = aylo::luau::FunctionRef;
using Value = aylo::luau::Value;
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

/// @brief Evaluates @p source, which must return a function, and wraps that function in a handle.
FunctionRef makeFunctionRef(const aylo::luau::Runtime& runtime, const std::string& source) {
    const auto script = runtime.load("function-ref", source);
    EXPECT_TRUE(script);
    script->closure().push(aylo::luau::detail::rawState(runtime));
    const Function loader = Function::fromRef(aylo::luau::Ref::pop(aylo::luau::detail::rawState(runtime)), "loader");
    auto results = loader.call();
    EXPECT_TRUE(results);
    EXPECT_EQ(results->values().size(), 1U);
    auto function = results->values()[0].tryAs<FunctionRef>();
    EXPECT_NE(function, nullptr);
    return *function;
}
}  // namespace

TEST(FunctionRef, FromStackReadsFunction) {
    const auto runtime = makeRuntime();
    const auto function = makeFunctionRef(runtime, "return function(a) return a * 2 end");

    ASSERT_TRUE(aylo::luau::detail::pushFunctionRef(aylo::luau::detail::rawState(runtime), function));
    const auto read = aylo::luau::detail::readFunctionRef(aylo::luau::detail::rawState(runtime));

    EXPECT_TRUE(read);
}

TEST(FunctionRef, FromStackFailsForOtherTypes) {
    const auto runtime = makeRuntime();
    ValuePack pushed{};
    pushed.add({1.0});
    ASSERT_EQ(aylo::luau::detail::pushValuePack(aylo::luau::detail::rawState(runtime), pushed).value(), 1U);

    const auto read = aylo::luau::detail::readFunctionRef(aylo::luau::detail::rawState(runtime));

    ASSERT_FALSE(read);
    EXPECT_EQ(read.error().kind(), ErrorKind::kTypeMismatch);
}

TEST(FunctionRef, FromStackThrowsForNullState) {
    EXPECT_THROW(std::ignore = aylo::luau::detail::readFunctionRef(nullptr), aylo::luau::exceptions::InvalidState);
}

TEST(FunctionRef, CopiesShareTheFunction) {
    const auto runtime = makeRuntime();
    const auto function = makeFunctionRef(runtime, "return function(a) return a * 2 end");

    const FunctionRef copy{function};

    EXPECT_TRUE(copy.ref().isValid());
    const Function callable = Function::fromRef(copy, "copy");
    ValuePack args{};
    args.add({4.0});
    const auto results = callable.call(args);
    ASSERT_TRUE(results);
    EXPECT_DOUBLE_EQ(results->values()[0].as<double>(), 8.0);
}

TEST(FunctionRef, PushFailsForOtherState) {
    const auto runtime = makeRuntime();
    const auto other = makeRuntime();
    const auto function = makeFunctionRef(runtime, "return function() end");

    const auto pushed = aylo::luau::detail::pushFunctionRef(aylo::luau::detail::rawState(other), function);

    ASSERT_FALSE(pushed);
    EXPECT_EQ(pushed.error().kind(), ErrorKind::kRuntime);
}

TEST(FunctionRef, MovedFromHandleThrows) {
    const auto runtime = makeRuntime();
    auto function = makeFunctionRef(runtime, "return function() end");
    const FunctionRef moved{std::move(function)};

    EXPECT_THROW(std::ignore = function.ref(), aylo::luau::exceptions::InvalidRef);
    EXPECT_THROW(std::ignore = aylo::luau::detail::pushFunctionRef(aylo::luau::detail::rawState(runtime), function),
                 aylo::luau::exceptions::InvalidRef);
}

TEST(FunctionRef, ValueHoldsFunctionRef) {
    const auto runtime = makeRuntime();
    const auto function = makeFunctionRef(runtime, "return function() return 1 end");

    const Value value{function};

    EXPECT_TRUE(value.is<FunctionRef>());
    const Value copy{value};
    EXPECT_TRUE(copy.is<FunctionRef>());
}

TEST(FunctionRef, ValuePushesFunctionBack) {
    const auto runtime = makeRuntime();
    const Value value{makeFunctionRef(runtime, "return function() return 1 end")};

    ASSERT_TRUE(aylo::luau::detail::pushValue(aylo::luau::detail::rawState(runtime), value));

    EXPECT_TRUE(aylo::luau::detail::readFunctionRef(aylo::luau::detail::rawState(runtime)));
}
