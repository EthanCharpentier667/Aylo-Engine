/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptParticlesTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "FakeBackend.hpp"
#include "ScriptProbe.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/particle/ParticleSystems.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
/// @brief A client App with the ParticlePlugin over the fake backend, 0.1 second per frame, and a script host on it.
struct ClientScripts {
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    ClientScripts() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{100}));
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        app.addPlugin(aylo::plugins::input::InputPlugin{});
        app.addPlugin(aylo::plugins::graphics::GraphicsPlugin{
            aylo::test::plugins::makeFakeRegistry(std::make_shared<aylo::test::plugins::FakeBackendState>()),
            aylo::test::plugins::makeFakeSettings()});
        app.addPlugin(aylo::plugins::shader::ShaderPlugin{});
        app.addPlugin(aylo::plugins::particle::ParticlePlugin{});
        app.getPluginRegistry().addScriptForms("test.Probe", aylo::tests::kProbeForms);
        auto created = aylo::scripting::ScriptHost::create(app, {.isClient = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("particles-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @return The emitters playing.
    [[nodiscard]] std::size_t playing() const {
        return app.getWorld().getResources().require<aylo::plugins::particle::ParticleSystems>().getCount();
    }
};
}  // namespace

TEST(ScriptParticles, AScriptPlaysAnEmitterAndAOneShotOfItsEffects) {
    ClientScripts client;
    ASSERT_EQ(client.run(R"(
        local smoke = engine.assets.particle({ name = "game.smoke", rate = 20, lifetime = 0.5 })
        local hit = engine.assets.particle({
            name = "game.hit", loop = false, duration = 0.1, lifetime = 0.2, bursts = { { count = 8 } },
        })

        local started = false
        engine.ecs.system("game.Effects", {
            phase = "Update",
            side = "client",
            query = {},
            events = { send = { "aylo.SpawnParticles" } },
            run = function(ctx)
                if started then return end
                started = true
                ctx.commands:spawn({
                    ["aylo.Transform2D"] = { position = { x = 10, y = 20 } },
                    ["aylo.ParticleEmitter"] = { effect = smoke, rateScale = 2 },
                })
                ctx.events.send("aylo.SpawnParticles", { effect = hit, position = { x = 50, y = 0 }, layer = 1 })
            end,
        })

        probe.finished = 0
        engine.ecs.system("game.Finished", {
            phase = "Update",
            side = "client",
            query = {},
            events = { read = { "aylo.ParticlesFinished" } },
            run = function(ctx)
                for _ in ctx.events.read("aylo.ParticlesFinished") do
                    probe.finished += 1
                end
            end,
        })
    )"),
              "");

    client.app.runFrame();  // the system spawns the emitter and asks for the one-shot
    client.app.runFrame();  // both play
    EXPECT_EQ(client.playing(), 2U);
    for (int frame = 0; frame < 10; ++frame) {
        client.app.runFrame();
    }
    EXPECT_EQ(client.playing(), 1U);  // the one-shot is over and gone; the looping emitter plays on
    EXPECT_EQ(client.run("assert(probe.finished == 1, tostring(probe.finished))"), "");
}

TEST(ScriptParticles, AnEmitterCanNameItsEffect) {
    ClientScripts client;
    ASSERT_EQ(client.run(R"(
        engine.assets.particle({ name = "game.smoke", rate = 20 })
        local started = false
        engine.ecs.system("game.Effects", {
            phase = "Update",
            side = "client",
            query = {},
            run = function(ctx)
                if started then return end
                started = true
                ctx.commands:spawn({
                    ["aylo.Transform2D"] = {},
                    ["aylo.ParticleEmitter"] = { effect = "game.smoke" },
                })
            end,
        })
    )"),
              "");
    client.app.runFrame();
    client.app.runFrame();
    EXPECT_EQ(client.playing(), 1U);
}

// The recipes of manual/pages/particles.md, as written there.
TEST(ScriptParticles, TheManualsRecipesRun) {
    ClientScripts client;
    ASSERT_EQ(client.run(R"(
        Game = {}
        engine.ecs.component("game.Laser", { fields = { speed = "f32" } })
        engine.ecs.component("game.Ship", { fields = { player = "u8" } })
        Game.effects = {
            sparks = engine.assets.particle({
                name = "game.sparks",
                texture = engine.assets.texture("particles/spark.png"),
                loop = false,
                duration = 0.1,
                bursts = { { count = 24 } },
                lifetime = { min = 0.2, max = 0.4 },
                speed = { min = 80, max = 160 },
                colorOverLife = { { t = 0, color = { 1, 0.9, 0.4 } }, { t = 1, color = { 1, 0.2, 0, 0 } } },
            }),
            trail = engine.assets.particle({
                name = "game.trail",
                texture = engine.assets.texture("particles/smoke.png"),
                blend = "alpha",
                rate = 40,
                lifetime = 0.6,
                angle = { min = math.pi - 0.2, max = math.pi + 0.2 },
                speed = 60,
                sizeOverLife = { { t = 0, value = 8 }, { t = 1, value = 24 } },
                colorOverLife = { { t = 0, color = { 1, 1, 1, 0.6 } }, { t = 1, color = { 1, 1, 1, 0 } } },
            }),
        }

        engine.ecs.system("game.MuzzleSparks", {
            phase = "Update",
            side = "client",
            query = { read = { "aylo.Transform2D" }, added = { "game.Laser" } },
            events = { send = { "aylo.SpawnParticles" } },
            run = function(ctx)
                for _, transform in ctx.query do
                    local at = transform.position
                    ctx.events.send("aylo.SpawnParticles", { effect = Game.effects.sparks, position = { x = at.x, y = at.y } })
                end
            end,
        })

        engine.ecs.system("game.ShipTrail", {
            phase = "Update",
            side = "client",
            query = { with = { "game.Ship" }, without = { "aylo.ParticleEmitter" } },
            run = function(ctx)
                for ship in ctx.query do
                    ctx.commands:add(ship, "aylo.ParticleEmitter", { effect = Game.effects.trail, layer = -1 })
                end
            end,
        })

        local spawned = false
        engine.ecs.system("game.Spawn", {
            phase = "PreUpdate",
            side = "client",
            query = {},
            run = function(ctx)
                if spawned then return end
                spawned = true
                ctx.commands:spawn({ ["game.Ship"] = {}, ["aylo.Transform2D"] = { position = { x = 1, y = 2 } } })
                ctx.commands:spawn({ ["game.Laser"] = {}, ["aylo.Transform2D"] = { position = { x = 3, y = 4 } } })
            end,
        })
    )"),
              "");

    for (int frame = 0; frame < 3; ++frame) {
        client.app.runFrame();
    }
    EXPECT_EQ(client.playing(), 2U);  // the ship's trail and the laser's sparks
}
