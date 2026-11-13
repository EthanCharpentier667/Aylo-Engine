/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BudgetTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <optional>
#include <stdexcept>
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

namespace {
using Clock = std::chrono::steady_clock;
using std::chrono_literals::operator""ms;

constexpr std::chrono::milliseconds kShortBudget = 50ms;
/// How late past its budget a stopped script may end: the interrupt reads the clock every few hundred back edges.
constexpr std::chrono::milliseconds kMargin = 500ms;
constexpr std::size_t kMemoryLimit = std::size_t{16} * 1024 * 1024;

aylo::luau::Runtime makeRuntime(const std::chrono::milliseconds callBudget, const std::size_t memoryLimit = 0,
                                const bool nativeCode = false) {
    auto runtime = aylo::luau::Runtime::create({
        .libs = aylo::luau::RuntimeConfig::Libs::kStandard,
        .optimizationLevel = aylo::luau::RuntimeConfig::OptimizationLevel::kStandard,
        .nativeCode = nativeCode,
        .memoryLimit = memoryLimit,
        .callBudget = callBudget,
    });
    EXPECT_TRUE(runtime);
    return std::move(*runtime);
}

/// @return The error kind of running @p source, or nothing when it ran to its end.
std::optional<aylo::luau::ErrorKind> runKind(const aylo::luau::Runtime& runtime, const std::string& source) {
    auto script = runtime.load("budget", source);
    EXPECT_TRUE(script);
    auto ran = runtime.run(*script);
    if (ran) {
        return std::nullopt;
    }
    return ran.error().kind();
}

/// @return Whether @p runtime still runs a script and gives its result.
bool stillRuns(const aylo::luau::Runtime& runtime) {
    auto script = runtime.load("after", "return 40 + 2");
    if (!script) {
        return false;
    }
    auto result = runtime.call(*script, {});
    return result && result->size() == 1 && result->values()[0].as<double>() == 42.0;
}

/// @brief Wraps the chunk of a Luau source in a Function.
aylo::luau::Function makeFunction(const aylo::luau::Runtime& runtime, const std::string& source) {
    const auto script = runtime.load("function", source);
    EXPECT_TRUE(script);
    script->closure().push(aylo::luau::detail::rawState(runtime));
    return aylo::luau::Function::fromRef(aylo::luau::Ref::pop(aylo::luau::detail::rawState(runtime)), "function");
}
}  // namespace

TEST(Budget, AnEndlessLoopStopsWithATimeoutAndTheRuntimeGoesOn) {
    const auto runtime = makeRuntime(kShortBudget);
    const auto start = Clock::now();

    EXPECT_EQ(runKind(runtime, "while true do end"), aylo::luau::ErrorKind::kTimeout);
    EXPECT_LT(Clock::now() - start, kShortBudget + kMargin);
    EXPECT_TRUE(stillRuns(runtime));
}

TEST(Budget, AnEndlessLoopStopsInMachineCodeToo) {
    const auto runtime = makeRuntime(kShortBudget, 0, true);
    const auto start = Clock::now();

    EXPECT_EQ(runKind(runtime, "local n = 0 while true do n += 1 end"), aylo::luau::ErrorKind::kTimeout);
    EXPECT_LT(Clock::now() - start, kShortBudget + kMargin);
    EXPECT_TRUE(stillRuns(runtime));
}

TEST(Budget, TheTimeoutSaysTheBudget) {
    const auto runtime = makeRuntime(kShortBudget);
    auto script = runtime.load("loop", "while true do end");
    ASSERT_TRUE(script);

    const auto ran = runtime.run(*script);

    ASSERT_FALSE(ran);
    EXPECT_NE(ran.error().message().find("ran longer than its budget of 50 ms"), std::string::npos)
        << ran.error().message();
}

TEST(Budget, AScriptCannotCatchItsTimeoutForever) {
    const auto runtime = makeRuntime(kShortBudget);

    EXPECT_EQ(runKind(runtime, "while true do pcall(function() while true do end end) end"),
              aylo::luau::ErrorKind::kTimeout);
    EXPECT_TRUE(stillRuns(runtime));
}

TEST(Budget, EachCallGetsAWholeBudget) {
    const auto runtime = makeRuntime(100ms);
    // Each run takes 60 ms: two in a row pass the budget together, never alone.
    const std::string busy = "local start = os.clock() while os.clock() - start < 0.06 do end";

    EXPECT_EQ(runKind(runtime, busy), std::nullopt);
    EXPECT_EQ(runKind(runtime, busy), std::nullopt);
}

TEST(Budget, NestedCallsShareTheOutermostDeadline) {
    const auto runtime = makeRuntime(100ms);
    // 30 ms each: under the budget alone, past it the fourth time in one outer call.
    const aylo::luau::Function inner =
        makeFunction(runtime, "local start = os.clock() while os.clock() - start < 0.03 do end");
    bool innerTimedOut = false;
    aylo::luau::Function native = aylo::luau::Function::create(runtime, "native");
    native.setHandler([&inner, &innerTimedOut](const aylo::luau::CallContext&) {
        auto called = inner.call({});
        if (!called) {
            innerTimedOut = innerTimedOut || called.error().kind() == aylo::luau::ErrorKind::kTimeout;
            throw std::runtime_error{std::string{called.error().message()}};
        }
        return aylo::luau::ValuePack{};
    });
    ASSERT_TRUE(runtime.globals().set("native", aylo::luau::Value{native.ref()}));

    EXPECT_EQ(runKind(runtime, "for i = 1, 10 do native() end"), aylo::luau::ErrorKind::kTimeout);
    EXPECT_TRUE(innerTimedOut);
    EXPECT_TRUE(stillRuns(runtime));
}

TEST(Budget, ALongScriptUnderItsBudgetRuns) {
    const auto runtime = makeRuntime(5000ms);

    EXPECT_EQ(runKind(runtime, "local n = 0 for i = 1, 1000000 do n += i end"), std::nullopt);
}

TEST(Budget, NoBudgetLetsAScriptRunAsLongAsItNeeds) {
    const auto runtime = makeRuntime(0ms);

    EXPECT_EQ(runKind(runtime, "local start = os.clock() while os.clock() - start < 0.1 do end"), std::nullopt);
}

TEST(Budget, AnAllocationBombStopsWithOutOfMemoryAndTheRuntimeGoesOn) {
    const auto runtime = makeRuntime(0ms, kMemoryLimit);

    EXPECT_EQ(runKind(runtime, "local t = {} for i = 1, 100000000 do t[i] = string.rep('x', 100) .. i end"),
              aylo::luau::ErrorKind::kOutOfMemory);
    EXPECT_LE(runtime.memoryInUse(), kMemoryLimit);
    EXPECT_EQ(runKind(runtime, "collectgarbage()"), std::nullopt);
    EXPECT_TRUE(stillRuns(runtime));
}

TEST(Budget, MemoryInUseCountsWhatTheScriptsHold) {
    const auto runtime = makeRuntime(0ms);
    const std::size_t before = runtime.memoryInUse();

    EXPECT_EQ(runKind(runtime, "big = string.rep('x', 1000000)"), std::nullopt);

    EXPECT_GE(runtime.memoryInUse(), before + 1000000);
}

TEST(Budget, DeepRecursionStopsWithStackOverflow) {
    const auto runtime = makeRuntime(0ms);

    EXPECT_EQ(runKind(runtime, "local function f(n) return f(n + 1) + 1 end return f(1)"),
              aylo::luau::ErrorKind::kStackOverflow);
    EXPECT_TRUE(stillRuns(runtime));
}

TEST(Budget, AnErrorOfTheScriptStaysARuntimeError) {
    const auto runtime = makeRuntime(kShortBudget, kMemoryLimit);

    EXPECT_EQ(runKind(runtime, "error('boom')"), aylo::luau::ErrorKind::kRuntime);
}
