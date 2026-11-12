/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** LibraryTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "aylo/luau/Function.hpp"
#include "aylo/luau/Library.hpp"
#include "aylo/luau/LibraryBuilder.hpp"
#include "aylo/luau/Runtime.hpp"
#include "aylo/luau/RuntimeConfig.hpp"
#include "aylo/luau/Value.hpp"
#include "aylo/luau/ValuePack.hpp"

namespace {
using Function = aylo::luau::Function;
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

/// @brief Binds @p function to a native function that returns its argument multiplied by @p factor.
void bindMultiplier(Function& function, const double factor) {
    function.bind([factor](const double value) {
        ValuePack results{};
        results.add({value * factor});
        return results;
    });
}

/// @brief Runs @p source, which must assign a number to the global `result`, and returns that number.
double runForResult(const aylo::luau::Runtime& runtime, const std::string& source) {
    const auto script = runtime.load("library-test", source);
    EXPECT_TRUE(script);
    EXPECT_TRUE(runtime.run(*script));
    const auto result = runtime.globals().get("result");
    EXPECT_TRUE(result);
    const auto* number = result->tryAs<double>();
    EXPECT_NE(number, nullptr);
    return *number;
}
}  // namespace

TEST(Library, KeepsItsName) {
    auto runtime = makeRuntime();
    aylo::luau::LibraryBuilder builder{runtime};
    builder.setName("mathx");

    const auto library = builder.build();

    EXPECT_EQ(library.name(), "mathx");
}

TEST(Library, StoresItsNameInTheTable) {
    auto runtime = makeRuntime();
    aylo::luau::LibraryBuilder builder{runtime};
    builder.setName("mathx");

    const auto library = builder.build();

    const auto name = library.table().get("__name");
    ASSERT_TRUE(name);
    const auto* text = name->tryAs<std::string>();
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(*text, "mathx");
}

TEST(Library, RegistersFunctionsUnderTheirName) {
    auto runtime = makeRuntime();
    aylo::luau::LibraryBuilder builder{runtime};
    builder.setName("mathx");
    bindMultiplier(builder.function("double"), 2.0);
    bindMultiplier(builder.function("triple"), 3.0);

    const auto library = builder.build();

    ASSERT_TRUE(runtime.globals().set("mathx", Value{library.table()}));
    EXPECT_EQ(runForResult(runtime, "result = mathx.double(4) + mathx.triple(4)"), 20.0);
}

TEST(Library, FunctionsOutliveTheBuilder) {
    auto runtime = makeRuntime();
    auto builder = std::make_unique<aylo::luau::LibraryBuilder>(runtime);
    builder->setName("mathx");
    bindMultiplier(builder->function("double"), 2.0);
    const auto library = builder->build();
    builder.reset();

    ASSERT_TRUE(runtime.globals().set("mathx", Value{library.table()}));
    EXPECT_EQ(runForResult(runtime, "result = mathx.double(21)"), 42.0);
}

TEST(Library, FunctionReferencesStayValidWhileAddingMore) {
    constexpr std::size_t kFunctionCount = 200;
    auto runtime = makeRuntime();
    aylo::luau::LibraryBuilder builder{runtime};
    builder.setName("many");

    std::vector<Function*> functions;
    for (std::size_t i = 0; i < kFunctionCount; ++i) {
        functions.push_back(&builder.function("f" + std::to_string(i)));
    }
    for (std::size_t i = 0; i < kFunctionCount; ++i) {
        bindMultiplier(*functions[i], static_cast<double>(i));
    }

    const auto library = builder.build();

    ASSERT_TRUE(runtime.globals().set("many", Value{library.table()}));
    EXPECT_EQ(runForResult(runtime, "result = many.f0(5) + many.f1(5) + many.f199(5)"), 1000.0);
}
