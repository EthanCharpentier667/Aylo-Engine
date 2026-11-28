/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptEventTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>

#include "aylo/core/reload/ReloadOutcome.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"

namespace {
using Status = aylo::core::reload::ReloadOutcome::Status;

/// @brief An App whose every frame lasts one second, and a host on it.
struct Fixture {
    aylo::ecs::App app;
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    Fixture() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::seconds{1}, aylo::ecs::FixedTimestep{1, 5}));
        auto created = aylo::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& file, const std::string& source) const {
        const auto result = host->run(file, source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};

/// @brief The manual's example: a hit sent by one system and taken by another.
constexpr std::string_view kManualHit = R"(
engine.ecs.component("game.Health", { fields = { value = "f32" } })
engine.ecs.event("game.Hit", { fields = { target = "entity", damage = "u32" } })

engine.ecs.system("game.Shoot", {
	phase = "Update",
	query = { with = { "game.Health" } },
	run = function(ctx)
		for enemy in ctx.query do
			ctx.events.send("game.Hit", { target = enemy, damage = 3 })
		end
	end,
})

engine.ecs.system("game.TakeHits", {
	phase = "PostUpdate",
	run = function(ctx)
		for hit in ctx.events.read("game.Hit") do
			local health = ctx:get(hit.target, "game.Health")
			if health then
				health.value -= hit.damage
			end
		end
	end,
})
)";
}  // namespace

TEST(ScriptEvent, ASystemSendsAnEventAnotherReads) {
    Fixture fixture;
    ASSERT_EQ(fixture.run("hits.luau", std::string{kManualHit}), "");
    ASSERT_EQ(fixture.run("spawn.luau", R"(
        engine.ecs.system("game.Spawn", {
            phase = "Startup",
            run = function(ctx) ctx.commands:spawn({ ["game.Health"] = { value = 10 } }) end,
        })
    )"),
              "");
    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    fixture.app.runFrame();
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
    const auto& components = fixture.app.getWorld().getComponents();
    const auto* layout = fixture.host->findComponent("game.Health");
    const auto entities = components.findPool(layout->id)->getEntities();
    ASSERT_EQ(entities.size(), 1U);
    float value = 0.F;
    std::memcpy(&value, components.get(entities.front(), layout->id).data(), sizeof(float));
    EXPECT_FLOAT_EQ(value, 4.F) << "two frames of 3 damage";
    EXPECT_TRUE(fixture.app.getWorld().getEvents().find("game.Hit").has_value());
}

TEST(ScriptEvent, RefusesABadDeclaration) {
    Fixture fixture;
    EXPECT_NE(
        fixture.run("a.luau", R"(engine.ecs.event("game.A", { fields = { x = "f32" }, replication = "everyChange" }))")
            .find("Event 'game.A': unknown option 'replication' (an event takes only 'fields')"),
        std::string::npos);
    EXPECT_NE(
        fixture.run("b.luau", R"(engine.ecs.event("game.B", { fields = { x = "float" } }))").find("Event 'game.B': "),
        std::string::npos);
    EXPECT_NE(fixture.run("c.luau", R"(engine.ecs.event("game.C", {}))").find("Event 'game.C': needs a 'fields' table"),
              std::string::npos);
    ASSERT_EQ(fixture.run("d.luau", R"(engine.ecs.component("game.D", { fields = {} }))"), "");
    EXPECT_NE(fixture.run("e.luau", R"(engine.ecs.event("game.D", { fields = {} }))"), "")
        << "a component has the name";
    ASSERT_EQ(fixture.run("f.luau", R"(engine.ecs.event("game.F", { fields = { x = "f32" } }))"), "");
    EXPECT_EQ(fixture.run("g.luau", R"(engine.ecs.event("game.F", { fields = { x = "f32" } }))"), "")
        << "the same declaration again";
}

TEST(ScriptEvent, AReloadKeepsTheSameEventAndRebuildsForAChangedOne) {
    Fixture fixture;
    ASSERT_EQ(fixture.run("events.luau", R"(engine.ecs.event("game.Hit", { fields = { damage = "u32" } }))"), "");

    auto outcome =
        fixture.host->reloadScript("events.luau", R"(engine.ecs.event("game.Hit", { fields = { damage = "u32" } }))");
    EXPECT_EQ(outcome.status, Status::kApplied) << outcome.message;

    outcome =
        fixture.host->reloadScript("events.luau", R"(engine.ecs.event("game.Hit", { fields = { damage = "f32" } }))");
    EXPECT_EQ(outcome.status, Status::kNeedsRebuild) << outcome.message;
    EXPECT_NE(outcome.message.find("event 'game.Hit' changed"), std::string::npos) << outcome.message;

    outcome = fixture.host->reloadScript("events.luau", "");
    EXPECT_EQ(outcome.status, Status::kNeedsRebuild) << outcome.message;
    EXPECT_NE(outcome.message.find("event 'game.Hit' is no longer declared by"), std::string::npos) << outcome.message;
}
