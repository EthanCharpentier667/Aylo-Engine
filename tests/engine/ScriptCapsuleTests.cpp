/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptCapsuleTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ScriptProbe.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryState.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"

namespace aylo::test::script_capsule {
struct Health {
    std::int32_t value;
};
AYLO_ECS_COMPONENT(Health, "test.scriptcapsule.Health", value);

struct Armor {
    std::int32_t value{0};
    std::int32_t extra{0};
};
AYLO_ECS_COMPONENT(Armor, "test.scriptcapsule.Armor", value, extra);
}  // namespace aylo::test::script_capsule

namespace capsule = aylo::plugins::capsule;
namespace sc = aylo::test::script_capsule;

namespace {
/// @brief A standalone App with the capsule plugin, "actors/bydo.actor" (Health 3, a "Gun" child), "scenes/one.scene"
/// and "scenes/two.scene" (a bydo each), and a script host on it.
struct ScriptApp {
    aylo::ecs::App app;
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    ScriptApp() {
        app.addPlugin(capsule::CapsulePlugin{});
        static_cast<void>(app.getWorld().getComponents().registerType<sc::Health>());
        static_cast<void>(app.getWorld().getComponents().registerType<sc::Armor>());
        auto& capsules = *app.getWorld().getResources().getMut<capsule::Capsules>();
        capsule::Capsule bydo{"actors/bydo.actor"};
        bydo.editNode(0).values.set(app.getWorld().getComponents(), sc::Health{.value = 3});
        bydo.editNode(0).values.set(app.getWorld().getComponents(), sc::Armor{.value = 1, .extra = 2});
        static_cast<void>(bydo.addNode("Gun", 0));
        capsules.define(std::move(bydo));
        for (const char* path : {"scenes/one.scene", "scenes/two.scene"}) {
            capsule::Capsule level{path};
            level.editNode(level.addNode("Bydo", 0)).instance = capsule::CapsuleRef{
                .id = capsule::CapsuleId::fromPath("actors/bydo.actor"), .path = "actors/bydo.actor"};
            capsules.define(std::move(level));
        }
        app.getPluginRegistry().addScriptForms("test.Probe", aylo::tests::kProbeForms);
        auto created = aylo::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("capsule-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptCapsule, ASystemSpawnsLoadsAndReloadsCapsules) {
    ScriptApp script;
    ASSERT_EQ(script.run(R"(
        frame = 0
        engine.ecs.system("game.Levels", {
            phase = "Update",
            query = {},
            calls = { "rtype.capsule.spawn", "rtype.capsule.load", "rtype.capsule.reload" },
            run = function(ctx)
                frame += 1
                if frame == 1 then
                    bydo = ctx.call("rtype.capsule.spawn", { capsule = "actors/bydo.actor" })
                    ctx.commands:add(bydo, "test.scriptcapsule.Health", { value = 10 })   -- over the capsule's
                    probe.first = ctx.call("rtype.capsule.load", { capsule = "scenes/one.scene" })
                elseif frame == 2 then
                    probe.second = ctx.call("rtype.capsule.load", { capsule = "scenes/two.scene" })
                elseif frame == 3 then
                    probe.reloaded = ctx.call("rtype.capsule.reload", { root = bydo })
                end
            end,
        })
    )"),
              "");
    aylo::ecs::World& world = script.app.getWorld();
    script.app.runFrame();
    EXPECT_EQ(world.getEntityCount(), 5U);  // the bydo and its gun, the first level, its bydo and gun
    script.app.runFrame();
    EXPECT_EQ(world.getEntityCount(), 5U);  // the first level replaced by the second
    script.app.runFrame();
    EXPECT_EQ(script.run("assert(probe.reloaded == true)"), "");

    const auto* loaded = world.getResources().get<capsule::LoadedCapsule>();
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->capsule, aylo::core::asset::AssetId::fromPath("scenes/two.scene"));
    EXPECT_EQ(script.run("assert(probe.first ~= probe.second)"), "");
}

TEST(ScriptCapsule, AnOverrideAddedAfterTheSpawnReplacesTheCapsulesValue) {
    ScriptApp script;
    ASSERT_EQ(script.run(R"(
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            query = {},
            calls = { "rtype.capsule.spawn" },
            run = function(ctx)
                if bydo then return end
                bydo = ctx.call("rtype.capsule.spawn", { capsule = "actors/bydo.actor" })
                ctx.commands:add(bydo, "test.scriptcapsule.Health", { value = 10 })
            end,
        })
    )"),
              "");
    script.app.runFrame();
    aylo::ecs::World& world = script.app.getWorld();
    std::vector<std::int32_t> health;
    const aylo::ecs::QueryState<const sc::Health> state{world.getComponents()};
    aylo::ecs::Query<const sc::Health> query{state, 0};
    for (auto [entity, value] : query) {
        health.push_back(value.value);
    }
    EXPECT_EQ(health, (std::vector<std::int32_t>{10}));  // the bydo's root, its Health replaced by the script
    EXPECT_EQ(world.getEntityCount(), 2U);
}

TEST(ScriptCapsule, ACallOnAnUnknownCapsuleFailsNamingIt) {
    ScriptApp script;
    const std::string error = script.run(R"(engine.call("rtype.capsule.spawn", { capsule = "actors/nothing.actor" }))");
    EXPECT_TRUE(error.contains("rtype.capsule.spawn")) << error;
    EXPECT_TRUE(error.contains("No capsule has the id")) << error;
}

TEST(ScriptCapsule, TheFormsSpawnLoadAndReloadInOneCallEach) {
    ScriptApp script;
    ASSERT_EQ(script.run(R"(
        frame = 0
        engine.ecs.system("game.Levels", {
            phase = "Update",
            query = {},
            calls = { "rtype.capsule.spawn", "rtype.capsule.load", "rtype.capsule.reload" },
            run = function(ctx)
                frame += 1
                if frame == 1 then
                    probe.bydo = engine.capsule.spawn(ctx, "actors/bydo.actor", {
                        ["test.scriptcapsule.Armor"] = { extra = 9 },   -- the capsule's value stays
                    })
                    probe.level = engine.capsule.load(ctx, "scenes/one.scene")
                elseif frame == 2 then
                    probe.reloaded = engine.capsule.reload(ctx, probe.bydo)
                    probe.notARoot = engine.capsule.reload(ctx, ctx.commands:spawn())
                end
            end,
        })
    )"),
              "");
    aylo::ecs::World& world = script.app.getWorld();
    script.app.runFrame();
    EXPECT_EQ(world.getEntityCount(), 5U);  // the bydo and its gun, the level, its bydo and gun
    std::vector<std::pair<std::int32_t, std::int32_t>> armors;
    const aylo::ecs::QueryState<const sc::Armor> state{world.getComponents()};
    aylo::ecs::Query<const sc::Armor> query{state, 0};
    for (auto [entity, armor] : query) {
        armors.emplace_back(armor.value, armor.extra);
    }
    std::ranges::sort(armors);
    EXPECT_EQ(armors,
              (std::vector<std::pair<std::int32_t, std::int32_t>>{{1, 2}, {1, 9}}));  // level's bydo, patched one
    EXPECT_EQ(world.getResources().get<capsule::LoadedCapsule>()->capsule,
              aylo::core::asset::AssetId::fromPath("scenes/one.scene"));
    EXPECT_EQ(script.run("assert(probe.level ~= nil and probe.bydo ~= nil)"), "");

    script.app.runFrame();
    EXPECT_EQ(script.run("assert(probe.reloaded == true and probe.notARoot == false)"), "");
}

TEST(ScriptCapsule, TheFormsCheckWhatTheyAreGiven) {
    ScriptApp script;
    ASSERT_EQ(script.run(R"(
        probe.errors = {}
        local function try(...)
            local done, message = pcall(...)
            table.insert(probe.errors, done and "" or tostring(message))
        end
        engine.ecs.system("game.Mistakes", {
            phase = "Update",
            query = {},
            calls = { "rtype.capsule.spawn" },
            run = function(ctx)
                if #probe.errors > 0 then return end
                try(engine.capsule.spawn, "actors/bydo.actor")
                try(engine.capsule.spawn, ctx, 42)
                try(engine.capsule.spawn, ctx, "actors/bydo.actor", { ["test.scriptcapsule.Armor"] = 3 })
                try(engine.capsule.spawn, ctx, "actors/nothing.actor")
                try(engine.capsule.load, ctx, "scenes/one.scene")
                try(engine.capsule.spawn, ctx, "actors/bydo.actor", { ["test.scriptcapsule.Armor"] = { extar = 1 } })
                try(engine.capsule.spawn, ctx, "actors/bydo.actor", {}, "more")
            end,
        })
    )"),
              "");
    script.app.runFrame();
    const std::vector<std::string> expected{
        "engine.capsule.spawn: expected the system's ctx first",
        "engine.capsule.spawn: expected the capsule's path second",
        "engine.capsule.spawn: overrides: 'test.scriptcapsule.Armor' must be a table of fields",
        "engine.capsule.spawn: ",  // the call's own error follows
        "engine.capsule.load: ",
        "engine.capsule.spawn: overrides: ",
        "engine.capsule.spawn: expected at most 3 argument(s), got 4",
    };
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const std::string lua = "local text = probe.errors[" + std::to_string(index + 1) +
                                "]; assert(string.find(text, \"" + expected[index] + "\", 1, true), text)";
        EXPECT_EQ(script.run(lua), "") << expected[index];
    }
    EXPECT_EQ(script.run(R"(assert(string.find(probe.errors[4], "No capsule has the id", 1, true), probe.errors[4]))"),
              "");
    EXPECT_EQ(script.run(R"(assert(string.find(probe.errors[6], "extar", 1, true), probe.errors[6]))"), "");
}
