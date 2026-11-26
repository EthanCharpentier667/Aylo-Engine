/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptBehaviourTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/core/reload/ReloadOutcome.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"

namespace {
/// @brief An App whose every frame lasts one second and runs one fixed tick of one second, and a host on it.
struct Fixture {
    aylo::ecs::App app;
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    Fixture() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::seconds{1}, aylo::ecs::FixedTimestep{1, 5}));
        auto created = aylo::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
        EXPECT_EQ(run("components.luau", R"(
            engine.ecs.component("game.Position", { fields = { x = "f32", y = "f32" } })
            engine.ecs.component("game.Health", { fields = { value = "f32" } })
        )"),
                  "");
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& file, const std::string& source) const {
        const auto result = host->run(file, source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @return The entities having the component @p name.
    [[nodiscard]] std::vector<aylo::ecs::Entity> having(const std::string_view name) const {
        const auto& components = app.getWorld().getComponents();
        const auto id = components.find(name);
        if (!id || components.findPool(*id) == nullptr) {
            return {};
        }
        const auto entities = components.findPool(*id)->getEntities();
        return {entities.begin(), entities.end()};
    }

    /// @brief Reads a float field of an entity's component.
    [[nodiscard]] float read(const aylo::ecs::Entity entity, const std::string_view component,
                             const std::string_view name) const {
        const auto* layout = host->findComponent(component);
        const auto bytes = app.getWorld().getComponents().get(entity, layout->id);
        const auto field = std::ranges::find(layout->fields, name, &aylo::ecs::FieldInfo::name);
        float value = 0.F;
        std::memcpy(&value, bytes.data() + field->offset, sizeof(float));
        return value;
    }

    /// @return Whether a system of this name is in the schedule of @p phase.
    [[nodiscard]] bool hasSystem(const aylo::ecs::Phase phase, const std::string_view name) const {
        return app.getSchedules().getSchedule(phase).contains(name);
    }
};

/// @brief Spawns two entities with `game.Spinner`, one of them with a speed of its own.
constexpr std::string_view kSpawnSpinners = R"(
    engine.ecs.system("game.SpawnSpinners", {
        phase = "Startup",
        run = function(ctx)
            ctx.commands:spawn({ ["game.Spinner"] = {}, ["game.Position"] = {} })
            ctx.commands:spawn({ ["game.Spinner"] = { speed = 10 }, ["game.Position"] = {} })
        end,
    })
)";

/// @brief The Behaviour of the manual's first example, on a position instead of a transform.
constexpr std::string_view kManualSpinner = R"(
local Spinner = engine.behaviour("game.Spinner", {
	fields = { speed = "f32", turns = "f32" },
	defaults = { speed = 90 },
	side = "client",
})

function Spinner:onUpdate(dt)
	local position = self:getComponent("game.Position")
	position.x += self.speed * dt
	self.turns += 1
end

return Spinner
)";
}  // namespace

TEST(ScriptBehaviour, CompilesToAComponentAndOneSystemPerHook) {
    Fixture fixture;
    ASSERT_EQ(fixture.run("spinner.luau", std::string{kManualSpinner}), "");
    ASSERT_EQ(fixture.run("spawn.luau", std::string{kSpawnSpinners}), "");

    EXPECT_NE(fixture.host->findComponent("game.Spinner"), nullptr);
    EXPECT_TRUE(fixture.hasSystem(aylo::ecs::Phase::kUpdate, "game.Spinner.onUpdate"));
    EXPECT_FALSE(fixture.hasSystem(aylo::ecs::Phase::kFixedUpdate, "game.Spinner.onFixedUpdate"))
        << "a hook it does not define has no system";

    fixture.app.runFrame();
    fixture.app.runFrame();
    std::vector<float> positions;
    for (const aylo::ecs::Entity entity : fixture.having("game.Spinner")) {
        positions.push_back(fixture.read(entity, "game.Position", "x"));
        EXPECT_FLOAT_EQ(fixture.read(entity, "game.Spinner", "turns"), 2.F);
    }
    std::ranges::sort(positions);
    EXPECT_EQ(positions, (std::vector<float>{20.F, 180.F}));
}

TEST(ScriptBehaviour, StartsEachEntityOnceBeforeItsOtherHooks) {
    Fixture fixture;
    ASSERT_EQ(fixture.run("spinner.luau", R"(
        local Spinner = engine.behaviour("game.Spinner", { fields = { speed = "f32", starts = "f32", ticks = "f32" } })
        function Spinner:onFixedUpdate(dt)
            assert(self.starts == 1, "onFixedUpdate before onStart")
            self.ticks += dt
        end
        function Spinner:onStart() -- defined after the other hook: still runs first
            self.starts += 1
        end
    )"),
              "");
    ASSERT_EQ(fixture.run("spawn.luau", std::string{kSpawnSpinners}), "");
    ASSERT_EQ(fixture.run("later.luau", R"(
        engine.ecs.system("game.SpawnLater", {
            phase = "FixedUpdate",
            run = function(ctx)
                if ctx.time.tick == 1 then
                    ctx.commands:spawn({ ["game.Spinner"] = {} }) -- started on the next frame
                end
            end,
        })
    )"),
              "");
    EXPECT_TRUE(fixture.hasSystem(aylo::ecs::Phase::kPreUpdate, "game.Spinner.onStart"));

    testing::internal::CaptureStderr();
    for (int frame = 0; frame < 4; ++frame) {
        fixture.app.runFrame();
    }
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
    const auto spinners = fixture.having("game.Spinner");
    ASSERT_EQ(spinners.size(), 3U);
    for (const aylo::ecs::Entity entity : spinners) {
        EXPECT_FLOAT_EQ(fixture.read(entity, "game.Spinner", "starts"), 1.F);
        EXPECT_GT(fixture.read(entity, "game.Spinner", "ticks"), 0.F);
    }
}

TEST(ScriptBehaviour, SelfReachesItsEntityAndTheOthers) {
    Fixture fixture;
    ASSERT_EQ(fixture.run("turret.luau", R"lua(
        engine.ecs.component("game.Barrel", { fields = { heat = "f32" } })
        engine.ecs.component("game.Hit", { fields = {} })
        local Turret = engine.behaviour("game.Turret", { fields = { target = "entity", barrels = "f32" } })

        function Turret:countBarrels()
            local count = 0
            for _, child in self:getChildren() do
                if child:hasComponent("game.Barrel") then
                    count += 1
                    child:getComponent("game.Barrel").heat = 5
                    assert(child:getParent() == self, "the parent of a child is the turret")
                    self.target = child -- an entity field takes a handle
                end
            end
            return count
        end

        function Turret:onUpdate()
            self.barrels = self:countBarrels()
            assert(self:getParent() == nil)
            assert(self.target == nil or typeof(self.target) == "Entity")
            if self:hasComponent("game.Hit") then
                self:removeComponent("game.Hit")
                self:addComponent("game.Health", { value = 3 })
            end
            local target = self:getHandle(self.target)
            assert(target.entity == self.target and tostring(target) == "Handle(" .. tostring(self.target) .. ")")
        end

        engine.ecs.system("game.Build", {
            phase = "Startup",
            run = function(ctx)
                local turret = ctx.commands:spawn({ ["game.Turret"] = {}, ["game.Hit"] = {} })
                for _ = 1, 2 do
                    ctx.commands:setParent(ctx.commands:spawn({ ["game.Barrel"] = {} }), turret)
                end
                ctx.commands:setParent(ctx.commands:spawn({ ["game.Position"] = {} }), turret)
            end,
        })
    )lua"),
              "");

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");

    const auto turrets = fixture.having("game.Turret");
    ASSERT_EQ(turrets.size(), 1U);
    EXPECT_FLOAT_EQ(fixture.read(turrets.front(), "game.Turret", "barrels"), 2.F);
    for (const aylo::ecs::Entity barrel : fixture.having("game.Barrel")) {
        EXPECT_FLOAT_EQ(fixture.read(barrel, "game.Barrel", "heat"), 5.F);
    }
    EXPECT_TRUE(fixture.having("game.Hit").empty()) << "removeComponent";
    ASSERT_EQ(fixture.having("game.Health").size(), 1U) << "addComponent";
    EXPECT_FLOAT_EQ(fixture.read(fixture.having("game.Health").front(), "game.Health", "value"), 3.F);
}

TEST(ScriptBehaviour, DestroyDespawnsAfterTheHook) {
    Fixture fixture;
    ASSERT_EQ(fixture.run("bullet.luau", R"(
        local Bullet = engine.behaviour("game.Bullet", { fields = { life = "f32" }, defaults = { life = 2 } })
        function Bullet:onFixedUpdate(dt)
            self.life -= dt
            if self.life <= 0 then
                self:destroy()
                assert(self:hasComponent("game.Bullet"), "destroying is deferred")
            end
        end
        engine.ecs.system("game.Shoot", {
            phase = "Startup",
            run = function(ctx) ctx.commands:spawn({ ["game.Bullet"] = {} }) end,
        })
    )"),
              "");
    fixture.app.runFrame();
    EXPECT_EQ(fixture.having("game.Bullet").size(), 1U);
    fixture.app.runFrame();
    EXPECT_TRUE(fixture.having("game.Bullet").empty());
}

TEST(ScriptBehaviour, SkipsTheEntitiesSwitchedOff) {
    Fixture fixture;
    ASSERT_EQ(fixture.run("spinner.luau", std::string{kManualSpinner}), "");
    ASSERT_EQ(fixture.run("spawn.luau", R"(
        engine.ecs.system("game.SpawnSpinners", {
            phase = "Startup",
            run = function(ctx)
                ctx.commands:spawn({ ["game.Spinner"] = {}, ["game.Position"] = {}, ["aylo.Disabled"] = {} })
            end,
        })
    )"),
              "");
    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.read(fixture.having("game.Spinner").front(), "game.Spinner", "turns"), 0.F);
}

TEST(ScriptBehaviour, RefusesStateOutsideItsFields) {
    Fixture fixture;
    ASSERT_EQ(fixture.run("spinner.luau", R"lua(
        local Spinner = engine.behaviour("game.Spinner", { fields = { speed = "f32" } })
        function Spinner:onUpdate()
            local ok, message = pcall(function() self.cooldown = 3 end)
            assert(not ok and string.find(message, "'game.Spinner' has no field 'cooldown': a Behaviour keeps its state in its declared fields", 1, true), message)
            ok, message = pcall(function() self.speed = self:getComponent("game.Position") end)
            assert(not ok and string.find(message, "cannot keep a view in self.speed: a view only lives during the hook, call self:getComponent in each hook", 1, true), message)
            ok, message = pcall(function() return self.nope end)
            assert(not ok and string.find(message, "'game.Spinner' has no field 'nope'", 1, true), message)
            ok, message = pcall(function() return self.getComponent("game.Position") end)
            assert(not ok and string.find(message, "getComponent is a method: call it with ':'", 1, true), message)
            ok, message = pcall(function() return self:getComponent(42) end)
            assert(not ok and string.find(message, "getComponent expects the name of a component", 1, true), message)
            ok, message = pcall(function() return self:getHandle("player") end)
            assert(not ok and string.find(message, "getHandle expects an entity", 1, true), message)
            self.speed += 1
        end
        engine.ecs.system("game.Spawn", {
            phase = "Startup",
            run = function(ctx) ctx.commands:spawn({ ["game.Spinner"] = {}, ["game.Position"] = {} }) end,
        })
    )lua"),
              "");
    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
    EXPECT_FLOAT_EQ(fixture.read(fixture.having("game.Spinner").front(), "game.Spinner", "speed"), 1.F);
}

TEST(ScriptBehaviour, RefusesABadDeclaration) {
    Fixture fixture;
    const auto failure = [&fixture](const std::string& source) { return fixture.run("bad.luau", source); };

    EXPECT_NE(failure(R"(engine.behaviour("game.A", { fields = { getParent = "f32" } }))")
                  .find("engine.behaviour 'game.A': the field 'getParent' has the name of a method of self"),
              std::string::npos);
    EXPECT_NE(failure(R"(engine.behaviour("game.B", { fields = { entity = "entity" } }))").find("the field 'entity'"),
              std::string::npos);
    EXPECT_NE(failure(R"(engine.behaviour("game.C", {}))").find("engine.behaviour 'game.C': 'fields' is required"),
              std::string::npos);
    EXPECT_NE(failure(R"(engine.behaviour("game.D", { fields = {}, phase = "Update" }))")
                  .find("engine.behaviour 'game.D': unexpected 'phase'"),
              std::string::npos);
    EXPECT_NE(failure(R"(engine.behaviour("game.E", { fields = {}, side = "everywhere" }))")
                  .find("'side' must be \"server\", \"client\" or \"both\""),
              std::string::npos);
    EXPECT_NE(failure(R"(engine.behaviour(42, { fields = {} }))").find("expected the name of the Behaviour first"),
              std::string::npos);
    EXPECT_NE(
        failure(R"(
        local F = engine.behaviour("game.F", { fields = {} })
        function F:onUpdat() end
    )")
            .find("engine.behaviour 'game.F': 'onUpdat' is not a hook (hooks are onStart, onUpdate, onFixedUpdate)"),
        std::string::npos);
    EXPECT_NE(failure(R"(
        local G = engine.behaviour("game.G", { fields = {} })
        function G:onUpdate() end
        function G:onUpdate() end
    )")
                  .find("engine.behaviour 'game.G': 'onUpdate' is defined twice"),
              std::string::npos);
    EXPECT_NE(failure(R"(
        local H = engine.behaviour("game.H", { fields = { speed = "f32" } })
        function H:speed() end
    )")
                  .find("'speed' is a field or a method of self"),
              std::string::npos);
    EXPECT_NE(failure(R"(
        local I = engine.behaviour("game.I", { fields = {} })
        I.onUpdate = 3
    )")
                  .find("'onUpdate' must be a function"),
              std::string::npos);
}

TEST(ScriptBehaviour, AHookThatFailsIsLoggedAndTheGameGoesOn) {
    Fixture fixture;
    ASSERT_EQ(fixture.run("broken.luau", R"(
        local Broken = engine.behaviour("game.Broken", { fields = {} })
        function Broken:onUpdate() error("boom") end
        engine.ecs.system("game.Spawn", {
            phase = "Startup",
            run = function(ctx) ctx.commands:spawn({ ["game.Broken"] = {}, ["game.Position"] = {} }) end,
        })
        engine.ecs.system("game.Move", {
            phase = "Update",
            query = { write = { "game.Position" } },
            run = function(ctx)
                for _, position in ctx.query do position.x += 1 end
            end,
        })
    )"),
              "");
    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    fixture.app.runFrame();
    const std::string errors = testing::internal::GetCapturedStderr();
    EXPECT_NE(errors.find("game.Broken.onUpdate"), std::string::npos) << errors;
    EXPECT_NE(errors.find("boom"), std::string::npos) << errors;
    EXPECT_FLOAT_EQ(fixture.read(fixture.having("game.Position").front(), "game.Position", "x"), 2.F);
}

TEST(ScriptBehaviour, AReloadReplacesTheHooksWithoutStartingAgain) {
    Fixture fixture;
    const auto spinner = [](const std::string_view step) {
        return R"(
            local Spinner = engine.behaviour("game.Spinner", { fields = { starts = "f32", turns = "f32" } })
            function Spinner:onStart() self.starts += 1 end
            function Spinner:onUpdate() self.turns += )" +
               std::string{step} + R"( end
        )";
    };
    ASSERT_EQ(fixture.run("spinner.luau", spinner("1")), "");
    ASSERT_EQ(fixture.run("spawn.luau", std::string{kSpawnSpinners}), "");
    fixture.app.runFrame();

    const auto outcome = fixture.host->reloadScript("spinner.luau", spinner("10"));
    ASSERT_EQ(outcome.status, aylo::core::reload::ReloadOutcome::Status::kApplied) << outcome.message;
    fixture.app.runFrame();

    for (const aylo::ecs::Entity entity : fixture.having("game.Spinner")) {
        EXPECT_FLOAT_EQ(fixture.read(entity, "game.Spinner", "starts"), 1.F);
        EXPECT_FLOAT_EQ(fixture.read(entity, "game.Spinner", "turns"), 11.F);
    }
}

TEST(ScriptBehaviour, AClientBehaviourReadsTheFieldsTheServerReplicates) {
    Fixture fixture;
    ASSERT_EQ(fixture.run("ship.luau", R"lua(
        local Ship = engine.behaviour("game.Ship", {
            fields = { speed = "f32" },
            defaults = { speed = 4 },
            replication = "everyChange",
            side = "client",
        })
        function Ship:onUpdate()
            self:getComponent("game.Position").x = self.speed -- a client-only use of the field
            local ok, message = pcall(function() self.speed = 1 end)
            assert(not ok and string.find(message, "read-only", 1, true), message)
        end
        engine.ecs.system("game.Spawn", {
            phase = "Startup",
            run = function(ctx) ctx.commands:spawn({ ["game.Ship"] = {}, ["game.Position"] = {} }) end,
        })
    )lua"),
              "");
    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
    EXPECT_FLOAT_EQ(fixture.read(fixture.having("game.Ship").front(), "game.Position", "x"), 4.F);
}
