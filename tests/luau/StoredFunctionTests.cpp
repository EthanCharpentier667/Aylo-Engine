/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StoredFunctionTests
*/

#include <gtest/gtest.h>
#include <lua.h>
#include <lualib.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/luau/ErrorKind.hpp"
#include "aylo/luau/Exceptions.hpp"
#include "aylo/luau/Function.hpp"
#include "aylo/luau/Runtime.hpp"
#include "aylo/luau/RuntimeConfig.hpp"
#include "aylo/luau/Table.hpp"
#include "aylo/luau/Userdata.hpp"
#include "aylo/luau/Usertype.hpp"
#include "aylo/luau/Value.hpp"
#include "aylo/luau/ValuePack.hpp"
#include "aylo/luau/detail/RawState.hpp"
#include "aylo/luau/detail/Stack.hpp"

namespace {
using Function = aylo::luau::Function;
using Userdata = aylo::luau::Userdata;
using Value = aylo::luau::Value;
using ValuePack = aylo::luau::ValuePack;

/// @brief A C++ type a stored function receives, as a system receives a component view.
struct Counter {
    double value{0};
};

/// @brief Creates a runtime with the standard libraries, failing the test if it cannot be created.
aylo::luau::Runtime makeRuntime() {
    auto runtime = aylo::luau::Runtime::create({
        .libs = aylo::luau::RuntimeConfig::Libs::kStandard,
        .optimizationLevel = aylo::luau::RuntimeConfig::OptimizationLevel::kStandard,
    });
    EXPECT_TRUE(runtime);
    return std::move(*runtime);
}

/// @brief Registers `Counter` with a read-write `value`.
aylo::luau::Usertype<Counter> registerCounter(aylo::luau::Runtime& runtime) {
    auto registered = runtime.usertype<Counter>("Counter");
    EXPECT_TRUE(registered);
    return registered->getter("value", [](const Counter& counter) { return counter.value; })
        .setter("value", [](Counter& counter, const double value) { counter.value = value; });
}

/// @brief Runs @p source as the script @p name, failing the test if it raises an error.
void run(const aylo::luau::Runtime& runtime, const std::string& name, const std::string& source) {
    const auto script = runtime.load(name, source);
    ASSERT_TRUE(script);
    const auto result = runtime.run(*script);
    ASSERT_TRUE(result) << result.error().message();
}

/// @brief Gets the function `run` of the global table `system`, as a system declaration holds it.
Function storedRun(const aylo::luau::Runtime& runtime) {
    const auto system = runtime.globals().get("system");
    EXPECT_TRUE(system);
    auto function = system->as<aylo::luau::Table>().function("run");
    EXPECT_TRUE(function);
    return std::move(*function);
}
}  // namespace

TEST(StoredFunction, IsCalledWithAUserdataAndANumber) {
    auto runtime = makeRuntime();
    const auto usertype = registerCounter(runtime);
    run(runtime, "systems.luau", R"(
        system = { run = function(counter, dt) counter.value = counter.value + dt end }
    )");
    const Function stored = storedRun(runtime);
    const auto userdata = usertype.create(Counter{.value = 1});
    ASSERT_TRUE(userdata);

    ValuePack args{};
    args.add({*userdata});
    args.add({0.5});
    ASSERT_TRUE(stored.call(args));

    EXPECT_EQ(userdata->tryAs<Counter>()->value, 1.5);
}

TEST(StoredFunction, KeepsWorkingAcrossFramesAndCollections) {
    auto runtime = makeRuntime();
    const auto usertype = registerCounter(runtime);
    run(runtime, "systems.luau", R"(
        local calls = 0
        system = { run = function(counter, dt) calls = calls + 1 counter.value = calls end }
    )");
    const Function stored = storedRun(runtime);
    const auto userdata = usertype.create(Counter{});
    ASSERT_TRUE(userdata);

    for (int frame = 1; frame <= 100; ++frame) {
        lua_gc(aylo::luau::detail::rawState(runtime), LUA_GCCOLLECT, 0);
        ValuePack args{};
        args.add({*userdata});
        args.add({0.0});
        ASSERT_TRUE(stored.call(args)) << "frame " << frame;
        ASSERT_EQ(userdata->tryAs<Counter>()->value, frame);
    }
}

TEST(StoredFunction, ReportsTheScriptNameAndTheLineOfAnError) {
    auto runtime = makeRuntime();
    const auto usertype = registerCounter(runtime);
    run(runtime, "Shield.luau", "system = {\n    run = function(counter, dt)\n        error(\"boom\")\n    end,\n}\n");
    const Function stored = storedRun(runtime);
    const auto userdata = usertype.create(Counter{});
    ASSERT_TRUE(userdata);

    ValuePack args{};
    args.add({*userdata});
    args.add({0.0});
    const auto result = stored.call(args);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kRuntime);
    EXPECT_EQ(result.error().message(), "Shield.luau:3: boom");
}

TEST(StoredFunction, ReportsTheLineOfAnErrorRaisedByAMethod) {
    auto runtime = makeRuntime();
    const auto usertype = registerCounter(runtime);
    run(runtime, "Shield.luau", "system = { run = function(counter, dt)\n    counter.value = \"x\"\nend }");
    const Function stored = storedRun(runtime);
    const auto userdata = usertype.create(Counter{});
    ASSERT_TRUE(userdata);

    ValuePack args{};
    args.add({*userdata});
    args.add({0.0});
    const auto result = stored.call(args);

    ASSERT_FALSE(result);
    EXPECT_NE(result.error().message().find("Shield.luau:2:"), std::string_view::npos) << result.error().message();
}

TEST(StoredFunction, IteratesOverANativeIterator) {
    auto runtime = makeRuntime();
    auto entities = std::make_shared<std::vector<double>>(std::vector<double>{10, 20, 30});
    auto next = Function::create(runtime, "query");
    next.setHandler([entities, index = std::size_t{0}](const aylo::luau::CallContext&) mutable -> ValuePack {
        ValuePack results{};
        if (index < entities->size()) {
            results.add({(*entities)[index++]});
        }
        return results;
    });
    ASSERT_TRUE(runtime.globals().set("query", Value{next.ref()}));

    run(runtime, "loop.luau", R"(
        sum = 0
        for value in query do
            sum = sum + value
        end
    )");

    const auto sum = runtime.globals().get("sum");
    ASSERT_TRUE(sum);
    EXPECT_EQ(sum->as<double>(), 60);
}

TEST(StoredFunction, OutlivesItsRuntimeWithoutTouchingTheClosedState) {
    std::optional<aylo::luau::Runtime> runtime{makeRuntime()};
    run(*runtime, "systems.luau", "system = { run = function() end }");
    const Function stored = storedRun(*runtime);
    const auto table = runtime->globals().get("system");
    ASSERT_TRUE(table);
    const auto ref = table->as<aylo::luau::Table>().functionRef("run");
    ASSERT_TRUE(ref);
    EXPECT_TRUE(stored.ref().isValid());

    runtime.reset();

    // The handles are destroyed after the state is closed: they must not unref into it.
    EXPECT_FALSE(stored.ref().isValid());
    EXPECT_FALSE(ref->isValid());
    const auto result = stored.call({});
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kRuntime);
    EXPECT_NE(result.error().message().find("runtime was destroyed"), std::string_view::npos);
}

TEST(StoredFunction, PushingItAfterItsRuntimeIsDestroyedFails) {
    auto first = std::optional<aylo::luau::Runtime>{makeRuntime()};
    run(*first, "systems.luau", "system = { run = function() end }");
    const auto system = first->globals().get("system");
    const auto ref = system->as<aylo::luau::Table>().functionRef("run");
    ASSERT_TRUE(ref);
    auto second = makeRuntime();

    first.reset();

    const auto pushed = aylo::luau::detail::pushFunctionRef(aylo::luau::detail::rawState(second), *ref);
    ASSERT_FALSE(pushed);
    EXPECT_EQ(pushed.error().kind(), aylo::luau::ErrorKind::kRuntime);
}

TEST(StoredFunction, ATableOfAClosedRuntimeIsRefused) {
    std::optional<aylo::luau::Runtime> runtime{makeRuntime()};
    run(*runtime, "systems.luau", "system = { run = function() end }");
    const auto system = runtime->globals().get("system");
    ASSERT_TRUE(system);
    const auto table = system->as<aylo::luau::Table>();

    runtime.reset();

    EXPECT_THROW(static_cast<void>(table.get("run")), aylo::luau::exceptions::InvalidRef);
}
