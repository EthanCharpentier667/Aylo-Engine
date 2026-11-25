/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptEntityComponentsTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/hierarchy/Children.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/schedule/Schedule.hpp"
#include "aylo/ecs/schedule/Schedules.hpp"
#include "aylo/ecs/system/System.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"

namespace aylo::test::entity_components {
/// @brief What the scripts below report, read by the tests.
struct Report {
    std::int32_t value;
    std::int32_t checks;
};
AYLO_ECS_RESOURCE(Report, "test.any.Report", value, checks);
}  // namespace aylo::test::entity_components

namespace {
using aylo::test::entity_components::Report;

/// @brief A standalone App with a manual clock, a report, a host, and an enemy with health targeted by a hunter.
struct Fixture {
    aylo::ecs::App app{aylo::ecs::RunMode::kStandalone};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    /// @param hierarchy Whether aylo.Parent and aylo.Children are registered from the start, as a plugin does.
    explicit Fixture(const bool hierarchy = true) {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::seconds{1}, aylo::ecs::FixedTimestep{1, 5}));
        app.getWorld().getResources().insert(Report{.value = 0, .checks = 0});
        if (hierarchy) {
            static_cast<void>(app.getWorld().getComponents().registerType<aylo::ecs::Parent>());
            static_cast<void>(app.getWorld().getComponents().registerType<aylo::ecs::Children>());
        }
        auto created = aylo::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
        EXPECT_EQ(run(R"(
            engine.ecs.component("game.Health", { fields = { value = "i32" } })
            engine.ecs.component("game.Target", { fields = { entity = "entity" } })
            engine.ecs.component("game.Armor", { fields = { value = "i32" } })
            engine.ecs.component("game.Synced", { fields = { value = "i32" }, replication = "everyChange" })
            engine.ecs.system("game.Spawn", {
                phase = "Startup",
                run = function(ctx)
                    local enemy = ctx.commands:spawn({
                        ["game.Health"] = { value = 10 },
                        ["game.Synced"] = { value = 4 },
                    })
                    ctx.commands:spawn({ ["game.Target"] = { entity = enemy } })
                end,
            })
        )"),
                  "");
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("entity-components-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    [[nodiscard]] const Report& report() const { return app.getWorld().getResources().require<Report>(); }

    /// @return The system of the Update phase with this name, or null.
    [[nodiscard]] const aylo::ecs::System* find(const std::string_view name) const {
        return app.getSchedules().getSchedule(aylo::ecs::Phase::kUpdate).findSystem(name);
    }
};
}  // namespace

TEST(ScriptEntityComponents, ASystemReadsAndWritesAComponentOfAnotherEntity) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Hit", {
            phase = "Update",
            query = { read = { "game.Target" } },
            run = function(ctx)
                for entity, target in ctx.query do
                    assert(ctx:has(target.entity, "game.Health"))
                    assert(not ctx:has(target.entity, "game.Armor"))
                    assert(ctx:get(target.entity, "game.Armor") == nil)
                    assert(not ctx:has(entity, "game.Health"))
                    local health = ctx:get(target.entity, "game.Health")
                    health.value -= 3 -- the first write makes it a component the system writes
                    local report = ctx.resources["test.any.Report"]
                    report.value = ctx:get(target.entity, "game.Health").value
                    report.checks += 1
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();
    fixture.app.runFrame();

    EXPECT_EQ(fixture.report().value, 4);  // 10, minus 3 on each frame
    EXPECT_EQ(fixture.report().checks, 2);
    const aylo::ecs::System* system = fixture.find("game.Hit");
    ASSERT_NE(system, nullptr);
    // The query reads game.Target; game.Health and game.Armor were found on top of it, and game.Health is written.
    const auto components = system->getAccess().getComponents();
    ASSERT_EQ(components.size(), 3U);
    EXPECT_EQ(system->getDeclaredAccess().getComponents().size(), 1U);
    EXPECT_TRUE(components[1].write);
    EXPECT_FALSE(components[2].write);
}

TEST(ScriptEntityComponents, ADeadEntityHasNoComponent) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Kill", {
            phase = "Update",
            query = { read = { "game.Target" } },
            run = function(ctx)
                for _, target in ctx.query do
                    local report = ctx.resources["test.any.Report"]
                    if report.checks == 0 then
                        ctx.commands:despawn(target.entity)
                    else
                        assert(ctx:get(target.entity, "game.Health") == nil)
                        assert(not ctx:has(target.entity, "game.Health"))
                    end
                    report.checks += 1
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();
    fixture.app.runFrame();

    EXPECT_EQ(fixture.report().checks, 2);
}

TEST(ScriptEntityComponents, WhatADeclarationSaysIsKept) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Declared", {
            phase = "Update",
            query = { read = { "game.Target" } },
            components = { read = { "game.Health" }, write = { "game.Armor" } },
            run = function(ctx)
                for _, target in ctx.query do
                    local ok, message = pcall(function() ctx:get(target.entity, "game.Health").value = 0 end)
                    assert(not ok and string.find(message, "components.read", 1, true), message)
                    ctx.resources["test.any.Report"].value = ctx:get(target.entity, "game.Health").value
                end
            end,
        })
    )"),
              "");

    const aylo::ecs::System* system = fixture.find("game.Declared");
    ASSERT_NE(system, nullptr);
    ASSERT_EQ(system->getDeclaredAccess().getComponents().size(), 3U);  // the query, then the components

    fixture.app.runFrame();

    EXPECT_EQ(fixture.report().value, 10);
}

TEST(ScriptEntityComponents, MistakesAreLuauErrors) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"lua(
        engine.ecs.system("game.Mistakes", {
            phase = "Update",
            query = { read = { "game.Target" } },
            run = function(ctx)
                for _, target in ctx.query do
                    local ok, message = pcall(function() return ctx:get(target.entity, "game.Nope") end)
                    assert(not ok and string.find(message, "unknown component 'game.Nope' (system 'game.Mistakes')",
                        1, true), message)
                    ok, message = pcall(function() return ctx.get(target.entity, "game.Health") end)
                    assert(not ok and string.find(message, "ctx:get expects an entity", 1, true), message)
                    ok, message = pcall(function() return ctx:has("game.Health") end)
                    assert(not ok and string.find(message, "ctx:has expects an entity", 1, true), message)
                    ctx.resources["test.any.Report"].checks += 1
                end
            end,
        })
    )lua"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.report().checks, 1);
}

TEST(ScriptEntityComponents, AViewOfAnotherEntityDiesWithTheRun) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        saved = nil
        engine.ecs.system("game.Keep", {
            phase = "Update",
            query = { read = { "game.Target" } },
            run = function(ctx)
                for _, target in ctx.query do
                    if saved == nil then
                        saved = ctx:get(target.entity, "game.Health")
                    else
                        local ok, message = pcall(function() return saved.value end)
                        assert(not ok and string.find(message, "no longer valid", 1, true), message)
                        ctx.resources["test.any.Report"].checks += 1
                    end
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();
    fixture.app.runFrame();

    EXPECT_EQ(fixture.report().checks, 1);
}

TEST(ScriptEntityComponents, AClientSystemReadsButCannotWriteAReplicatedComponent) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.ClientLook", {
            phase = "Update",
            side = "client",
            query = { read = { "game.Target" } },
            run = function(ctx)
                for _, target in ctx.query do
                    local synced = ctx:get(target.entity, "game.Synced")
                    assert(synced.value == 4)
                    local ok, message = pcall(function() synced.value = 5 end)
                    assert(not ok and string.find(message, "a client system cannot write 'game.Synced'", 1, true),
                        message)
                    assert(ctx:get(target.entity, "game.Synced").value == 4)
                    ctx.resources["test.any.Report"].checks += 1
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.report().checks, 1);
}

TEST(ScriptEntityComponents, TheHierarchyIsReadButNotWrittenThroughAView) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"lua(
        engine.ecs.system("game.Adopt", {
            phase = "Update",
            query = { read = { "game.Target" } },
            run = function(ctx)
                for entity, target in ctx.query do
                    local report = ctx.resources["test.any.Report"]
                    if report.checks == 0 then
                        ctx.commands:setParent(entity, target.entity)
                    else
                        local parent = ctx:get(entity, "aylo.Parent")
                        assert(parent.entity == target.entity)
                        local ok, message = pcall(function() parent.entity = entity end)
                        assert(not ok and string.find(message, "'aylo.Parent' cannot be written: use commands:setParent()",
                            1, true), message)
                        assert(ctx:get(entity, "aylo.Parent").entity == target.entity)
                    end
                    report.checks += 1
                end
            end,
        })
    )lua"),
              "");

    fixture.app.runFrame();
    fixture.app.runFrame();

    EXPECT_EQ(fixture.report().checks, 2);
    const aylo::ecs::System* system = fixture.find("game.Adopt");
    ASSERT_NE(system, nullptr);
    const auto components = system->getAccess().getComponents();
    ASSERT_EQ(components.size(), 2U);
    EXPECT_FALSE(components[1].write);  // the refused write did not give the system write access
}

TEST(ScriptEntityComponents, BadComponentDeclarationsAreRefusedWhenTheScriptLoads) {
    Fixture fixture;
    EXPECT_NE(fixture
                  .run(R"(
        engine.ecs.system("game.Unknown", { phase = "Update", components = { read = { "game.Nope" } },
            run = function(ctx) end })
    )")
                  .find("unknown component 'game.Nope'"),
              std::string::npos);
    EXPECT_NE(fixture
                  .run(R"(
        engine.ecs.system("game.Reparent", { phase = "Update", components = { write = { "aylo.Parent" } },
            run = function(ctx) end })
    )")
                  .find("use commands:setParent()"),
              std::string::npos);
    EXPECT_NE(fixture
                  .run(R"(
        engine.ecs.system("game.Typo", { phase = "Update", components = { edit = { "game.Health" } },
            run = function(ctx) end })
    )")
                  .find("unknown key 'edit'"),
              std::string::npos);
}

TEST(ScriptEntityComponents, ASystemWalksTheHierarchy) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"lua(
        kept = nil
        engine.ecs.system("game.Family", {
            phase = "Update",
            query = { read = { "game.Target" } },
            run = function(ctx)
                for hunter, target in ctx.query do
                    local report = ctx.resources["test.any.Report"]
                    if report.checks == 0 then
                        assert(ctx:getParent(hunter) == nil)
                        assert(#ctx:getChildren(target.entity) == 0)
                        ctx.commands:setParent(hunter, target.entity)
                        local second = ctx.commands:spawn({ ["game.Armor"] = { value = 2 } })
                        ctx.commands:setParent(second, target.entity)
                    elseif kept then
                        assert(#kept == 2 and kept[1] == hunter, "the list is a copy that outlives the run")
                    else
                        assert(ctx:getParent(hunter) == target.entity)
                        assert(ctx:getParent(target.entity) == nil)
                        local children = ctx:getChildren(target.entity)
                        assert(#children == 2 and children[1] == hunter, tostring(#children))
                        for _, child in children do
                            local armor = ctx:get(child, "game.Armor")
                            if armor then
                                report.value = armor.value
                            end
                        end
                        kept = children
                    end
                    report.checks += 1
                end
            end,
        })
    )lua"),
              "");

    fixture.app.runFrame();
    fixture.app.runFrame();
    fixture.app.runFrame();

    EXPECT_EQ(fixture.report().checks, 3);
    EXPECT_EQ(fixture.report().value, 2);
    const aylo::ecs::System* system = fixture.find("game.Family");
    ASSERT_NE(system, nullptr);
    // The query reads game.Target; aylo.Parent, aylo.Children and game.Armor were found, all read.
    const auto components = system->getAccess().getComponents();
    ASSERT_EQ(components.size(), 4U);
    EXPECT_TRUE(std::ranges::none_of(components, [](const auto& component) { return component.write; }));
}

TEST(ScriptEntityComponents, TheHierarchyOfAWorldWithNoParentYetIsEmpty) {
    Fixture fixture{false};
    ASSERT_EQ(fixture.run(R"lua(
        engine.ecs.system("game.Orphans", {
            phase = "Update",
            query = { read = { "game.Target" } },
            run = function(ctx)
                for hunter, target in ctx.query do
                    assert(ctx:getParent(hunter) == nil)
                    assert(#ctx:getChildren(target.entity) == 0)
                    local ok, message = pcall(function() return ctx:getParent("hunter") end)
                    assert(not ok and string.find(message, "ctx:getParent expects an entity", 1, true), message)
                    ok, message = pcall(function() return ctx.getChildren(hunter) end)
                    assert(not ok and string.find(message, "ctx:getChildren expects an entity", 1, true), message)
                    ctx.resources["test.any.Report"].checks += 1
                end
            end,
        })
    )lua"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.report().checks, 1);
}

/// @brief The example of manual/pages/systems.md, "The hierarchy", as written there.
constexpr const char* kManualBarrels = R"(
engine.ecs.system("game.CoolBarrels", {
	phase = "Update",
	query = { with = { "game.Turret" } },
	run = function(ctx)
		for turret in ctx.query do
			for _, child in ctx:getChildren(turret) do
				local barrel = ctx:get(child, "game.Barrel")
				if barrel then
					barrel.heat = math.max(barrel.heat - ctx.delta, 0)
				end
			end
		end
	end,
})
)";

TEST(ScriptEntityComponents, TheManualsTurretCoolsItsBarrels) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.component("game.Turret", { fields = {} })
        engine.ecs.component("game.Barrel", { fields = { heat = "f32" } })
        engine.ecs.system("game.Build", {
            phase = "Startup",
            run = function(ctx)
                local turret = ctx.commands:spawn({ ["game.Turret"] = {} })
                for _, heat in { 5, 1 } do
                    ctx.commands:setParent(ctx.commands:spawn({ ["game.Barrel"] = { heat = heat } }), turret)
                end
                ctx.commands:setParent(ctx.commands:spawn({ ["game.Armor"] = { value = 1 } }), turret)
            end,
        })
        engine.ecs.system("game.Look", {
            phase = "PostUpdate",
            query = { read = { "game.Barrel" } },
            run = function(ctx)
                local total = 0
                for _, barrel in ctx.query do
                    total += barrel.heat
                end
                ctx.resources["test.any.Report"].value = total
            end,
        })
    )"),
              "");
    ASSERT_EQ(fixture.run(kManualBarrels), "");

    fixture.app.runFrame();
    fixture.app.runFrame();

    EXPECT_EQ(fixture.report().value, 3);  // 5 and 1, cooled by one second on each frame: 3 and 0
}
