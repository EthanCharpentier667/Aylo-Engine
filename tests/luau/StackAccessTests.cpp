/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StackAccessTests
*/

#include <gtest/gtest.h>

#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "aylo/luau/Exceptions.hpp"
#include "aylo/luau/Function.hpp"
#include "aylo/luau/Result.hpp"
#include "aylo/luau/Runtime.hpp"
#include "aylo/luau/RuntimeConfig.hpp"
#include "aylo/luau/StackAccess.hpp"
#include "aylo/luau/Usertype.hpp"
#include "aylo/luau/Value.hpp"

namespace {
/// @brief A C++ object scripts read and write through raw handlers.
struct Point {
    double x{0};
    double y{0};
    std::string seen;  ///< The keys of the last table written, in a stable order.
};

constexpr std::array<std::string_view, 2> kKeys{"x", "y"};

/// @return A runtime with the standard libraries, interpreted or compiled to machine code.
aylo::luau::Runtime makeRuntime(const bool nativeCode = false) {
    auto runtime = aylo::luau::Runtime::create({
        .libs = aylo::luau::RuntimeConfig::Libs::kStandard,
        .optimizationLevel = aylo::luau::RuntimeConfig::OptimizationLevel::kStandard,
        .nativeCode = nativeCode,
    });
    EXPECT_TRUE(runtime);
    return std::move(*runtime);
}

/// @brief Registers `Point`: `x` and `y` read and written raw, `pos` read as a table `{x, y}` and written from one,
/// `bad` pushing nothing.
aylo::luau::Usertype<Point> registerPoint(aylo::luau::Runtime& runtime) {
    return aylo::luau::expect(runtime.usertype<Point>("Point"))
        .rawIndex([](const Point& point, const std::string_view key, aylo::luau::Pusher& result) {
            if (key == "x") {
                result.number(point.x);
            } else if (key == "y") {
                result.number(point.y);
            } else if (key == "pos") {
                const std::array numbers{point.x, point.y};
                result.record(kKeys, numbers);
            } else if (key == "bad") {
                return;  // pushes nothing
            } else {
                throw aylo::luau::exceptions::RuntimeError{std::format("no member '{}'", key)};
            }
        })
        .rawNewIndex([](Point& point, const std::string_view key, const aylo::luau::StackSlot& value) {
            if (key == "pos") {
                std::map<std::string, double, std::less<>> entries;
                value.forEachEntry(
                    [&entries](const std::optional<std::string_view> name, const aylo::luau::StackSlot& entry) {
                        entries.emplace(std::string{name.value_or("?")}, entry.number().value_or(-1));
                        return true;
                    });
                point.seen.clear();
                for (const auto& [name, number] : entries) {
                    point.seen += name;
                }
                point.x = entries["x"];
                point.y = entries["y"];
                return;
            }
            const std::optional<double> number = value.number();
            if (!number) {
                throw aylo::luau::exceptions::RuntimeError{"expected a number"};
            }
            (key == "x" ? point.x : point.y) = *number;
        });
}

/// @brief Runs a script, failing the test if it fails.
void run(const aylo::luau::Runtime& runtime, const std::string& source) {
    auto script = runtime.load("test", source);
    ASSERT_TRUE(script) << script.error().message();
    const auto result = runtime.run(*script);
    ASSERT_TRUE(result) << result.error().message();
}

/// @return The message of the error a script raises, empty if it raises none.
std::string runForError(const aylo::luau::Runtime& runtime, const std::string& source) {
    auto script = runtime.load("test", source);
    if (!script) {
        return std::string{script.error().message()};
    }
    const auto result = runtime.run(*script);
    return result ? std::string{} : std::string{result.error().message()};
}

/// @return A global number of the runtime.
double globalNumber(const aylo::luau::Runtime& runtime, const std::string_view name) {
    const auto value = runtime.globals().get(name);
    EXPECT_TRUE(value && value->is<double>()) << name;
    return value && value->is<double>() ? value->as<double>() : 0;
}

/// @brief Puts a new Point at (1, 2) in the global `point`, and returns its type.
aylo::luau::Usertype<Point> givePoint(aylo::luau::Runtime& runtime) {
    auto type = registerPoint(runtime);
    auto point = type.create(Point{.x = 1, .y = 2});
    EXPECT_TRUE(point);
    EXPECT_TRUE(runtime.globals().set("point", aylo::luau::Value{std::move(*point)}));
    return type;
}
}  // namespace

TEST(StackAccess, ARawIndexPushesNumbersAndTables) {
    auto runtime = makeRuntime();
    givePoint(runtime);
    run(runtime, R"(
        sum = point.x + point.y
        local pos = point.pos
        posX, posY = pos.x, pos.y
        pos.x = 10  -- a new table each time: changing it changes nothing else
        again = point.pos.x
    )");
    EXPECT_EQ(globalNumber(runtime, "sum"), 3);
    EXPECT_EQ(globalNumber(runtime, "posX"), 1);
    EXPECT_EQ(globalNumber(runtime, "posY"), 2);
    EXPECT_EQ(globalNumber(runtime, "again"), 1);
}

TEST(StackAccess, ARawNewIndexReadsNumbersAndVisitsTablesInPlace) {
    auto runtime = makeRuntime();
    givePoint(runtime);
    run(runtime, R"(
        point.x = 5
        point.pos = { y = 7, x = point.x * 2 }
        x, y = point.x, point.y
    )");
    EXPECT_EQ(globalNumber(runtime, "x"), 10);
    EXPECT_EQ(globalNumber(runtime, "y"), 7);
}

TEST(StackAccess, AStackSlotTellsANumberFromAStringThatLooksLikeOne) {
    auto runtime = makeRuntime();
    givePoint(runtime);
    EXPECT_TRUE(runForError(runtime, "point.x = '5'").contains("expected a number"));
}

TEST(StackAccess, ARawIndexThatPushesNothingRaisesAnError) {
    auto runtime = makeRuntime();
    givePoint(runtime);
    const std::string error = runForError(runtime, "local value = point.bad");
    EXPECT_TRUE(error.contains("reading 'bad' gave 0 values")) << error;
}

TEST(StackAccess, ARawIndexErrorNamesTheTypeAndTheMember) {
    auto runtime = makeRuntime();
    givePoint(runtime);
    const std::string error = runForError(runtime, "local value = point.z");
    EXPECT_TRUE(error.contains("Point.z")) << error;
    EXPECT_TRUE(error.contains("no member 'z'")) << error;
}

TEST(StackAccess, AGetterStillComesBeforeTheRawIndex) {
    auto runtime = makeRuntime();
    givePoint(runtime).getter("x", [](const Point& /*point*/) { return 42.0; });
    run(runtime, "x = point.x");
    EXPECT_EQ(globalNumber(runtime, "x"), 42);
}

TEST(StackAccess, ARawHandlerFeedsAGenericFor) {
    auto runtime = makeRuntime();
    auto type = registerPoint(runtime);
    int step = 0;
    auto iterator = aylo::luau::Function::create(runtime, "points");
    iterator.setRawHandler([&](aylo::luau::Pusher& results) {
        if (step == 3) {
            return;  // nothing: the loop ends
        }
        ++step;
        results.number(step);
        results.userdata(type, Point{.x = step * 10.0});
    });
    ASSERT_TRUE(runtime.globals().set("points", aylo::luau::Value{iterator.ref()}));
    run(runtime, R"(
        total = 0
        for index, point in points do
            total += index + point.x
        end
    )");
    EXPECT_EQ(globalNumber(runtime, "total"), 66);  // 1+10 + 2+20 + 3+30
}

TEST(StackAccess, ARawHandlerErrorNamesTheFunction) {
    auto runtime = makeRuntime();
    auto iterator = aylo::luau::Function::create(runtime, "broken");
    iterator.setRawHandler(
        [](aylo::luau::Pusher& /*results*/) { throw aylo::luau::exceptions::RuntimeError{"out of order"}; });
    ASSERT_TRUE(runtime.globals().set("broken", aylo::luau::Value{iterator.ref()}));
    const std::string error = runForError(runtime, "broken()");
    EXPECT_TRUE(error.contains("native function 'broken': out of order")) << error;
}

TEST(StackAccess, APusherGrowsTheStackPastWhatLuauGuarantees) {
    auto runtime = makeRuntime();
    auto many = aylo::luau::Function::create(runtime, "many");
    many.setRawHandler([](aylo::luau::Pusher& results) {
        for (int index = 0; index < 100; ++index) {
            results.number(index);
        }
    });
    ASSERT_TRUE(runtime.globals().set("many", aylo::luau::Value{many.ref()}));
    run(runtime, "count = select('#', many())");
    EXPECT_EQ(globalNumber(runtime, "count"), 100);
}

TEST(NativeCode, IsOffUnlessAsked) { EXPECT_FALSE(makeRuntime().isNative()); }

TEST(NativeCode, RunsScriptsAsTheInterpreterDoes) {
    for (const bool native : {false, true}) {
        auto runtime = makeRuntime(native);
        givePoint(runtime);
        run(runtime, R"(
            local function fib(n) if n < 2 then return n end return fib(n - 1) + fib(n - 2) end
            result = fib(20) + point.x
        )");
        EXPECT_EQ(globalNumber(runtime, "result"), 6766) << native;
        EXPECT_TRUE(runForError(runtime, "error('boom')").contains("test:1: boom")) << native;
    }
}
