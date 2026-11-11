/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptingPluginTests
*/

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>

#include "ScriptsDirectory.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/exceptions/ScriptLoadException.hpp"
#include "aylo/scripting/plugin/ScriptingPlugin.hpp"

namespace {
using aylo::tests::ScriptsDirectory;

/// @brief Adds the plugin to an App that runs one frame per second.
void addPlugin(aylo::ecs::App& app, aylo::scripting::ScriptingPlugin::Config config) {
    app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::seconds{1}, aylo::ecs::FixedTimestep{1, 5}));
    app.addPlugin(aylo::scripting::ScriptingPlugin{std::move(config)});
}

/// @brief Adds the plugin with the default entry (`main`).
void addPlugin(aylo::ecs::App& app, const std::filesystem::path& directory,
               const aylo::scripting::ScriptHost::Options options = {}) {
    addPlugin(app, {.directory = directory, .options = options});
}

/// @return The scripts' log of a component named @p name: whether the App has it.
bool hasComponent(aylo::ecs::App& app, const std::string& name) {
    return app.getWorld().getComponents().find(name).has_value();
}
}  // namespace

TEST(ScriptingPlugin, RunsOnlyTheEntriesAndWhatTheyRequire) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-reachable"};
    scripts.write("main.luau", R"(require("./nested/deep/b"))");
    scripts.write("nested/deep/b.luau", R"(engine.ecs.component("game.B", { fields = {} }))");
    scripts.write("notes.txt", "this is not a script and is not a Luau chunk");

    aylo::ecs::App app;
    addPlugin(app, scripts.path());

    EXPECT_TRUE(app.hasPlugin("rtype.scripting"));
    EXPECT_TRUE(hasComponent(app, "game.B"));
}

TEST(ScriptingPlugin, FilesNotRequiredAreNotRun) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-unrequired"};
    scripts.write("main.luau", R"(engine.ecs.component("game.Main", { fields = {} }))");
    scripts.write("other.luau", R"(engine.ecs.component("game.Other", { fields = {} }))");
    scripts.write("broken.luau", R"(error("never run"))");

    aylo::ecs::App app;
    addPlugin(app, scripts.path());

    EXPECT_TRUE(hasComponent(app, "game.Main"));
    EXPECT_FALSE(hasComponent(app, "game.Other"));
}

TEST(ScriptingPlugin, EntriesRunMainThenServerThenClient) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-order"};
    // The entries record their order in a module's state, which each one checks: only the right order passes.
    scripts.write("order.luau", R"(
        local steps = {}
        return {
            step = function(name, after) assert(steps[#steps] == after, name .. " ran after " .. tostring(steps[#steps])); steps[#steps + 1] = name end,
        }
    )");
    scripts.write("shared/main.luau", R"(require("@scripts/order").step("main", nil))");
    scripts.write("server/main.luau", R"(require("@scripts/order").step("server", "main"))");
    scripts.write("client/main.luau", R"(
        require("@scripts/order").step("client", "server")
        engine.ecs.component("game.Done", { fields = {} })
    )");

    aylo::ecs::App app;
    addPlugin(app, {.directory = scripts.path(),
                    .main = "shared/main",
                    .server = "server/main",
                    .client = "client/main",
                    .options = {.isClient = true, .isServer = true}});

    EXPECT_TRUE(hasComponent(app, "game.Done"));
}

TEST(ScriptingPlugin, ASideRunsOnlyItsOwnEntry) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-sides"};
    scripts.write("main.luau", R"(engine.ecs.component("game.Main", { fields = {} }))");
    scripts.write("server.luau", R"(engine.ecs.component("game.Server", { fields = {} }))");
    scripts.write("client.luau", R"(engine.ecs.component("game.Client", { fields = {} }))");

    aylo::ecs::App server;
    addPlugin(server, {.directory = scripts.path(),
                       .main = "main",
                       .server = "server",
                       .client = "client",
                       .options = {.isServer = true}});
    EXPECT_TRUE(hasComponent(server, "game.Main"));
    EXPECT_TRUE(hasComponent(server, "game.Server"));
    EXPECT_FALSE(hasComponent(server, "game.Client"));

    aylo::ecs::App client;
    addPlugin(client, {.directory = scripts.path(),
                       .main = "main",
                       .server = "server",
                       .client = "client",
                       .options = {.isClient = true}});
    EXPECT_TRUE(hasComponent(client, "game.Main"));
    EXPECT_FALSE(hasComponent(client, "game.Server"));
    EXPECT_TRUE(hasComponent(client, "game.Client"));
}

TEST(ScriptingPlugin, StandaloneRunsBothSidesAndSharedModulesOnce) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-standalone"};
    // Declaring a component twice with the same layout is fine, but a system name is unique: it fails if run twice.
    scripts.write("shared.luau", R"(
        engine.ecs.system("game.Shared", { phase = "Update", query = {}, run = function(ctx) end })
        return { value = 1 }
    )");
    scripts.write("server.luau", R"(
        assert(require("./shared").value == 1)
        engine.ecs.component("game.Server", { fields = {} })
    )");
    scripts.write("client.luau", R"(
        assert(require("./shared").value == 1)
        engine.ecs.component("game.Client", { fields = {} })
    )");

    aylo::ecs::App app;
    EXPECT_NO_THROW(addPlugin(app, {.directory = scripts.path(),
                                    .server = "server",
                                    .client = "client",
                                    .options = {.isClient = true, .isServer = true}}));
    EXPECT_TRUE(hasComponent(app, "game.Server"));
    EXPECT_TRUE(hasComponent(app, "game.Client"));
}

TEST(ScriptingPlugin, DefaultEntryIsMain) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-default"};
    scripts.write("main.luau", R"(engine.ecs.component("game.Main", { fields = {} }))");

    aylo::ecs::App app;
    addPlugin(app, scripts.path(), {.isClient = true, .isServer = true});

    EXPECT_TRUE(hasComponent(app, "game.Main"));
}

TEST(ScriptingPlugin, MissingDefaultEntryWarns) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-no-main"};
    scripts.write("other.luau", R"(error("never run"))");

    aylo::ecs::App app;
    testing::internal::CaptureStderr();
    EXPECT_NO_THROW(addPlugin(app, scripts.path()));
    const std::string warning = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(app.hasPlugin("rtype.scripting"));
    EXPECT_NE(warning.find("main"), std::string::npos) << warning;
}

TEST(ScriptingPlugin, MissingExplicitEntryFails) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-no-server"};
    scripts.write("main.luau", "");

    aylo::ecs::App app;
    try {
        addPlugin(app, {.directory = scripts.path(), .server = "server/main", .options = {.isServer = true}});
        FAIL() << "the plugin should have thrown";
    } catch (const aylo::scripting::exceptions::ScriptLoadException& e) {
        EXPECT_NE(std::string{e.what()}.find("scripts.server"), std::string::npos) << e.what();
        EXPECT_NE(std::string{e.what()}.find("server/main"), std::string::npos) << e.what();
    }
}

TEST(ScriptingPlugin, AnEntryOfAnotherSideMayBeMissing) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-other-side"};
    scripts.write("main.luau", R"(engine.ecs.component("game.Main", { fields = {} }))");

    aylo::ecs::App app;
    EXPECT_NO_THROW(addPlugin(
        app, {.directory = scripts.path(), .main = "main", .server = "server/main", .options = {.isClient = true}}));
    EXPECT_TRUE(hasComponent(app, "game.Main"));
}

TEST(ScriptingPlugin, EntryMayReturnNothing) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-returns-nothing"};
    scripts.write("main.luau", "local x = 1");

    aylo::ecs::App app;
    EXPECT_NO_THROW(addPlugin(app, scripts.path()));
}

TEST(ScriptingPlugin, ScriptsDoNotShareGlobals) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-globals"};
    scripts.write("main.luau", R"(
        shared = "from main"
        require("./other")
    )");
    scripts.write("other.luau", R"(
        assert(shared == nil, "a global leaked from another file")
        engine.ecs.component("game.Other", { fields = {} })
    )");

    aylo::ecs::App app;
    addPlugin(app, scripts.path());

    EXPECT_TRUE(hasComponent(app, "game.Other"));
}

TEST(ScriptingPlugin, ScriptsCannotModifyEngineOrItsTables) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-readonly"};
    scripts.write("main.luau", R"(
        assert(not pcall(function() engine.isServer = true end), "engine is writable")
        assert(not pcall(function() engine.mine = 1 end), "engine accepts new keys")
        assert(not pcall(function() engine.ecs.component = nil end), "engine.ecs is writable")
        assert(not pcall(function() engine.assets.x = 1 end), "engine.assets is writable")
        assert(not pcall(function() math.x = 1 end), "math is writable")
        assert(not pcall(function() string.x = 1 end), "string is writable")
        engine.ecs.component("game.Intact", { fields = {} })
    )");

    aylo::ecs::App app;
    addPlugin(app, scripts.path());

    EXPECT_TRUE(hasComponent(app, "game.Intact"));
}

TEST(ScriptingPlugin, ScriptsSeeTheFlagsOfTheConfig) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-flags"};
    scripts.write("main.luau", R"(
        if engine.isServer and not engine.isClient then
            engine.ecs.component("game.ServerOnly", { fields = {} })
        end
    )");

    aylo::ecs::App app;
    addPlugin(app, scripts.path(), {.isClient = false, .isServer = true});

    EXPECT_TRUE(app.getWorld().getComponents().find("game.ServerOnly"));
}

TEST(ScriptingPlugin, ASystemLoadedFromAFileRuns) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-system"};
    scripts.write("main.luau", R"(
        engine.ecs.component("game.Shield", { fields = { strength = "f32" }, defaults = { strength = 10 } })
        engine.ecs.system("game.Drain", {
            phase = "FixedUpdate",
            query = { write = { "game.Shield" } },
            run = function(ctx)
                for entity, shield in ctx.query do
                    shield.strength = shield.strength - 1
                end
            end,
        })
    )");

    aylo::ecs::App app;
    addPlugin(app, scripts.path());

    auto& components = app.getWorld().getComponents();
    const auto id = components.find("game.Shield");
    ASSERT_TRUE(id);
    const auto ship = app.getWorld().spawn();
    const float strength = 10.F;
    std::array<std::byte, sizeof(float)> bytes{};
    std::memcpy(bytes.data(), &strength, sizeof(float));
    ASSERT_TRUE(components.add(ship, *id, bytes));

    app.runFrame();

    float drained = 0.F;
    std::memcpy(&drained, components.get(ship, *id).data(), sizeof(float));
    EXPECT_FLOAT_EQ(drained, 9.F);
}

TEST(ScriptingPlugin, AMissingDirectoryLoadsNothingAndWarns) {
    aylo::ecs::App app;
    testing::internal::CaptureStderr();
    EXPECT_NO_THROW(addPlugin(app, std::filesystem::temp_directory_path() / "aylo-scripting-plugin-missing"));
    const std::string warning = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(app.hasPlugin("rtype.scripting"));
    EXPECT_NE(warning.find("aylo-scripting-plugin-missing"), std::string::npos) << warning;
}

TEST(ScriptingPlugin, AFailingScriptIsAnErrorNamingTheFile) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-failing"};
    scripts.write("main.luau", R"(error("boom"))");

    aylo::ecs::App app;
    try {
        addPlugin(app, scripts.path());
        FAIL() << "the plugin should have thrown";
    } catch (const aylo::scripting::exceptions::ScriptLoadException& e) {
        EXPECT_NE(std::string{e.what()}.find("main.luau"), std::string::npos) << e.what();
        EXPECT_NE(std::string{e.what()}.find("boom"), std::string::npos) << e.what();
    }
}

TEST(ScriptingPlugin, AClientSystemWritingAReplicatedComponentIsAnErrorNamingTheFile) {
    const ScriptsDirectory scripts{"aylo-scripting-plugin-client-write"};
    scripts.write("main.luau", R"(
        engine.ecs.component("game.Score", { fields = { points = "i32" }, replication = "everyChange" })
        engine.ecs.system("game.Hud", { phase = "Update", side = "client", query = { write = { "game.Score" } },
                                        run = function(ctx) end })
    )");

    aylo::ecs::App app;
    try {
        addPlugin(app, scripts.path());
        FAIL() << "the plugin should have thrown";
    } catch (const aylo::scripting::exceptions::ScriptLoadException& e) {
        EXPECT_NE(std::string{e.what()}.find("main.luau"), std::string::npos) << e.what();
        EXPECT_NE(std::string{e.what()}.find("game.Hud"), std::string::npos) << e.what();
        EXPECT_NE(std::string{e.what()}.find("game.Score"), std::string::npos) << e.what();
    }
}
