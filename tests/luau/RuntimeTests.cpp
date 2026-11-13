/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RuntimeTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>

#include "aylo/luau/ErrorKind.hpp"
#include "aylo/luau/FunctionRef.hpp"
#include "aylo/luau/Runtime.hpp"
#include "aylo/luau/RuntimeConfig.hpp"
#include "aylo/luau/Value.hpp"
#include "aylo/luau/ValuePack.hpp"

namespace {
constexpr aylo::luau::RuntimeConfig kStandardConfig{
    .libs = aylo::luau::RuntimeConfig::Libs::kStandard,
    .optimizationLevel = aylo::luau::RuntimeConfig::OptimizationLevel::kStandard,
};

aylo::luau::Runtime makeRuntime(const aylo::luau::RuntimeConfig config = kStandardConfig) {
    auto runtime = aylo::luau::Runtime::create(config);
    EXPECT_TRUE(runtime);
    return std::move(*runtime);
}

/// @brief Writes a temporary file removed on destruction.
class TempFile {
  public:
    TempFile(const std::string& name, const std::string& content)
        : _path{std::filesystem::temp_directory_path() / name} {
        std::ofstream{_path, std::ios::binary} << content;
    }

    ~TempFile() { std::filesystem::remove(_path); }
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    TempFile(TempFile&&) = delete;
    TempFile& operator=(TempFile&&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const { return _path; }

  private:
    std::filesystem::path _path;
};
}  // namespace

TEST(Runtime, CallGivesTheScriptItsArgumentsAndReturnsItsValues) {
    const auto runtime = makeRuntime();
    auto script = runtime.load("sum", "local a, b = ...; return a + b, 'done'");
    ASSERT_TRUE(script);
    aylo::luau::ValuePack arguments;
    arguments.add(aylo::luau::Value{2.0});
    arguments.add(aylo::luau::Value{3.0});

    const auto returned = runtime.call(*script, arguments);

    ASSERT_TRUE(returned);
    ASSERT_EQ(returned->size(), 2U);
    EXPECT_DOUBLE_EQ(returned->values()[0].as<double>(), 5.0);
    EXPECT_EQ(returned->values()[1].as<std::string>(), "done");
}

TEST(Runtime, CallReturnsTheErrorOfTheScript) {
    const auto runtime = makeRuntime();
    auto script = runtime.load("boom", "error('broken', 0)");
    ASSERT_TRUE(script);

    const auto returned = runtime.call(*script, {});

    ASSERT_FALSE(returned);
    EXPECT_EQ(returned.error().kind(), aylo::luau::ErrorKind::kRuntime);
    EXPECT_NE(returned.error().message().find("broken"), std::string_view::npos);
}

TEST(Runtime, CreateReturnsRuntime) {
    const auto runtime = aylo::luau::Runtime::create(kStandardConfig);

    EXPECT_TRUE(runtime);
}

TEST(Runtime, CreateWithNoLibrariesSucceeds) {
    const auto runtime = aylo::luau::Runtime::create({
        .libs = aylo::luau::RuntimeConfig::Libs::kNone,
        .optimizationLevel = aylo::luau::RuntimeConfig::OptimizationLevel::kDisabled,
    });

    EXPECT_TRUE(runtime);
}

TEST(Runtime, CreateWithPartialLibrariesOpensOnlyThose) {
    auto runtime = aylo::luau::Runtime::create({
        .libs = aylo::luau::RuntimeConfig::Libs::kBase | aylo::luau::RuntimeConfig::Libs::kMath,
        .optimizationLevel = aylo::luau::RuntimeConfig::OptimizationLevel::kStandard,
    });

    ASSERT_TRUE(runtime);
    const auto globals = runtime->globals();
    EXPECT_TRUE(aylo::luau::expect(globals.has("math")));
    EXPECT_FALSE(aylo::luau::expect(globals.has("table")));
}

TEST(Runtime, IsMovable) {
    auto runtime = makeRuntime();

    aylo::luau::Runtime moved{std::move(runtime)};
    auto other = makeRuntime();
    other = std::move(moved);

    const auto script = other.load("moved", "local x = 1");
    EXPECT_TRUE(script);
}

TEST(Runtime, LoadValidSourceReturnsScript) {
    const auto runtime = makeRuntime();

    const auto script = runtime.load("valid", "local x = 1 + 1");

    ASSERT_TRUE(script);
    EXPECT_EQ(script->name(), "valid");
    EXPECT_NE(script->state(), nullptr);
}

TEST(Runtime, LoadSyntaxErrorReturnsCompilationError) {
    const auto runtime = makeRuntime();

    const auto script = runtime.load("broken", "local = = 1");

    ASSERT_FALSE(script);
    EXPECT_EQ(script.error().kind(), aylo::luau::ErrorKind::kCompilation);
}

TEST(Runtime, CompilationErrorMessageEndsWithTheErrorAndNothingMore) {
    const auto runtime = makeRuntime();

    // The error is at the end of the source, so the message ends where the buffer does.
    const auto script = runtime.load("broken", "local x = (");

    ASSERT_FALSE(script);
    const std::string_view message = script.error().message();
    EXPECT_TRUE(message.starts_with("unable to compile script: "));
    EXPECT_TRUE(message.ends_with("<eof>")) << message;
}

TEST(Runtime, LoadDoesNotRunTheScript) {
    const auto runtime = makeRuntime();

    const auto script = runtime.load("lazy", "error('should not run')");

    EXPECT_TRUE(script);
}

TEST(Runtime, LoadFromFileNamesScriptAfterFilename) {
    const auto runtime = makeRuntime();
    const TempFile file{"aylo_runtime_tests_load.luau", "local x = 1"};

    const auto script = runtime.load(file.path());

    ASSERT_TRUE(script);
    EXPECT_EQ(script->name(), "aylo_runtime_tests_load.luau");
}

TEST(Runtime, LoadMissingFileReturnsError) {
    const auto runtime = makeRuntime();

    const auto script = runtime.load(std::filesystem::temp_directory_path() / "aylo_runtime_tests_missing.luau");

    ASSERT_FALSE(script);
    EXPECT_EQ(script.error().kind(), aylo::luau::ErrorKind::kUnknown);
}

TEST(Runtime, LoadFileWithSyntaxErrorReturnsCompilationError) {
    const auto runtime = makeRuntime();
    const TempFile file{"aylo_runtime_tests_broken.luau", "local = = 1"};

    const auto script = runtime.load(file.path());

    ASSERT_FALSE(script);
    EXPECT_EQ(script.error().kind(), aylo::luau::ErrorKind::kCompilation);
}

TEST(Runtime, RunValidScriptSucceeds) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("ok", "local x = 1 + 1");
    ASSERT_TRUE(script);

    EXPECT_TRUE(runtime.run(*script));
}

TEST(Runtime, RunClosureSucceeds) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("ok", "local x = 1");
    ASSERT_TRUE(script);

    EXPECT_TRUE(runtime.run(script->closure()));
}

TEST(Runtime, RunTwiceSucceeds) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("twice", "local x = 1");
    ASSERT_TRUE(script);

    EXPECT_TRUE(runtime.run(*script));
    EXPECT_TRUE(runtime.run(*script));
}

TEST(Runtime, RunRaisingScriptReturnsRuntimeError) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("raise", "error('boom')");
    ASSERT_TRUE(script);

    const auto result = runtime.run(*script);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kRuntime);
    EXPECT_NE(result.error().message().find("boom"), std::string_view::npos);
}

TEST(Runtime, RunAfterFailureStillWorks) {
    const auto runtime = makeRuntime();
    const auto failing = runtime.load("raise", "error('boom')");
    const auto working = runtime.load("ok", "local x = 1");
    ASSERT_TRUE(failing);
    ASSERT_TRUE(working);

    EXPECT_FALSE(runtime.run(*failing));
    EXPECT_TRUE(runtime.run(*working));
}

TEST(Runtime, StandardLibrariesAreAvailable) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("libs", "assert(math.max(1, 2) == 2 and string.len('ab') == 2)");
    ASSERT_TRUE(script);

    EXPECT_TRUE(runtime.run(*script));
}

TEST(Runtime, NoLibrariesLeavesGlobalsEmpty) {
    const auto runtime = makeRuntime({
        .libs = aylo::luau::RuntimeConfig::Libs::kNone,
        .optimizationLevel = aylo::luau::RuntimeConfig::OptimizationLevel::kStandard,
    });
    const auto script = runtime.load("nolibs", "local m = math\nlocal _ = m.max");
    ASSERT_TRUE(script);

    const auto result = runtime.run(*script);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kRuntime);
}

TEST(Runtime, ScriptRunsInItsOwnThread) {
    const auto runtime = makeRuntime();
    const auto first = runtime.load("first", "local x = 1");
    const auto second = runtime.load("second", "local x = 2");
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    EXPECT_NE(first->state(), second->state());
}

TEST(Runtime, ScriptGlobalsDoNotLeakToOtherScripts) {
    auto runtime = makeRuntime();
    runtime.sandbox();
    const auto writer = runtime.load("writer", "shared = 42");
    const auto reader = runtime.load("reader", "assert(shared == nil)");
    ASSERT_TRUE(writer);
    ASSERT_TRUE(reader);

    EXPECT_TRUE(runtime.run(*writer));
    EXPECT_TRUE(runtime.run(*reader));
}

TEST(Runtime, AnUnsandboxedRuntimeSharesGlobals) {
    const auto runtime = makeRuntime();
    const auto writer = runtime.load("writer", "shared = 42");
    const auto reader = runtime.load("reader", "assert(shared == 42)");
    ASSERT_TRUE(writer);
    ASSERT_TRUE(reader);

    EXPECT_FALSE(runtime.isSandboxed());
    EXPECT_TRUE(runtime.run(*writer));
    EXPECT_TRUE(runtime.run(*reader));
}

TEST(Runtime, SandboxMarksTheRuntimeSandboxed) {
    auto runtime = makeRuntime();

    runtime.sandbox();

    EXPECT_TRUE(runtime.isSandboxed());
}

TEST(Runtime, SandboxIsIdempotent) {
    auto runtime = makeRuntime();

    runtime.sandbox();
    runtime.sandbox();

    const auto script = runtime.load("script", "local x = math.max(1, 2)\nassert(x == 2)");
    ASSERT_TRUE(script);
    EXPECT_TRUE(runtime.isSandboxed());
    EXPECT_TRUE(runtime.run(*script));
}

TEST(Runtime, GlobalsSetBeforeSandboxAreSeenByEveryScript) {
    auto runtime = makeRuntime();
    ASSERT_TRUE(runtime.globals().set("engine", aylo::luau::Value{42.0}));
    runtime.sandbox();
    const auto first = runtime.load("first", "assert(engine == 42)");
    const auto second = runtime.load("second", "assert(engine == 42)");
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    EXPECT_TRUE(runtime.run(*first));
    EXPECT_TRUE(runtime.run(*second));
}

TEST(Runtime, AScriptGlobalStaysInItsThread) {
    auto runtime = makeRuntime();
    runtime.sandbox();
    const auto script = runtime.load("script", "mine = 7");
    ASSERT_TRUE(script);

    EXPECT_TRUE(runtime.run(*script));

    const auto value = runtime.globals().get("mine");
    ASSERT_TRUE(value);
    EXPECT_TRUE(value->is<aylo::luau::Value::Nil>());
}

TEST(Runtime, SettingAGlobalAfterSandboxFails) {
    auto runtime = makeRuntime();
    runtime.sandbox();

    const auto result = runtime.globals().set("late", aylo::luau::Value{1.0});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kRuntime);
    const auto value = runtime.globals().get("late");
    ASSERT_TRUE(value);
    EXPECT_TRUE(value->is<aylo::luau::Value::Nil>());
}

TEST(Runtime, ScriptKeepsItsOwnGlobalsBetweenRuns) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("counter", "count = (count or 0) + 1\nassert(count <= 2)");
    ASSERT_TRUE(script);

    EXPECT_TRUE(runtime.run(*script));
    EXPECT_TRUE(runtime.run(*script));
}

TEST(Runtime, ScriptCannotModifyStandardLibraries) {
    auto runtime = makeRuntime();
    runtime.sandbox();
    const auto script = runtime.load("tamper", "math.max = nil");
    ASSERT_TRUE(script);

    const auto result = runtime.run(*script);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), aylo::luau::ErrorKind::kRuntime);
}

TEST(Runtime, AllOptimizationLevelsRunScripts) {
    for (const auto level : {aylo::luau::RuntimeConfig::OptimizationLevel::kDisabled,
                             aylo::luau::RuntimeConfig::OptimizationLevel::kStandard,
                             aylo::luau::RuntimeConfig::OptimizationLevel::kAggressive}) {
        const auto runtime =
            makeRuntime({.libs = aylo::luau::RuntimeConfig::Libs::kStandard, .optimizationLevel = level});
        const auto script = runtime.load("opt", "local function f(a) return a * 2 end assert(f(2) == 4)");
        ASSERT_TRUE(script);

        EXPECT_TRUE(runtime.run(*script));
    }
}

TEST(Runtime, LoadsAPrecompiledChunkAsItsSource) {
    const auto runtime = makeRuntime();
    const auto chunk = runtime.precompile("local a, b = ...; return a * b");
    ASSERT_TRUE(chunk);
    EXPECT_TRUE(std::string_view{*chunk}.starts_with(aylo::luau::Runtime::kBytecodeHeader));
    auto script = runtime.load("product", *chunk);
    ASSERT_TRUE(script);
    aylo::luau::ValuePack arguments;
    arguments.add(aylo::luau::Value{4.0});
    arguments.add(aylo::luau::Value{5.0});

    const auto returned = runtime.call(*script, arguments);

    ASSERT_TRUE(returned);
    ASSERT_EQ(returned->size(), 1U);
    EXPECT_DOUBLE_EQ(returned->values()[0].as<double>(), 20.0);
}

TEST(Runtime, APrecompiledChunkKeepsTheLinesOfItsErrors) {
    const auto runtime = makeRuntime();
    const auto chunk = runtime.precompile("local x = 1\nerror('broken')");
    ASSERT_TRUE(chunk);
    auto script = runtime.load("lines.luau", *chunk);
    ASSERT_TRUE(script);

    const auto returned = runtime.call(*script, {});

    ASSERT_FALSE(returned);
    EXPECT_NE(returned.error().message().find("lines.luau:2"), std::string::npos) << returned.error().message();
}

TEST(Runtime, PrecompileReturnsTheCompilationError) {
    const auto runtime = makeRuntime();
    const auto chunk = runtime.precompile("local = ");

    ASSERT_FALSE(chunk);
    EXPECT_EQ(chunk.error().kind(), aylo::luau::ErrorKind::kCompilation);
}

TEST(Runtime, CollectGarbageCountsAndCollectsFromAScript) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("gc", R"(
        local garbage = {}
        for i = 1, 10000 do garbage[i] = { i } end
        garbage = nil
        local before = collectgarbage("count")
        local collected = collectgarbage()
        local after = collectgarbage("count")
        return type(before) == "number" and before > 0, collected, after < before
    )");
    ASSERT_TRUE(script) << script.error().message();
    const auto results = runtime.call(*script, {});
    ASSERT_TRUE(results) << results.error().message();
    ASSERT_EQ(results->size(), 3U);
    EXPECT_TRUE(results->values()[0].as<bool>());
    EXPECT_EQ(results->values()[1].as<double>(), 0.0);
    EXPECT_TRUE(results->values()[2].as<bool>());
}

TEST(Runtime, CollectGarbageRefusesAnUnknownOption) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("gc", "collectgarbage('step')");
    ASSERT_TRUE(script);
    const auto result = runtime.run(*script);
    ASSERT_FALSE(result);
    EXPECT_TRUE(std::string{result.error().message()}.contains("unknown option 'step'")) << result.error().message();
}

TEST(Runtime, CollectGarbageExistsOnlyWithTheBaseLibrary) {
    const auto bare = aylo::luau::Runtime::create({.libs = aylo::luau::RuntimeConfig::Libs::kMath});
    ASSERT_TRUE(bare);
    EXPECT_FALSE(bare->globals().get("collectgarbage")->is<aylo::luau::FunctionRef>());
}
