/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptScreenEffectsTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "FakeBackend.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/shader/ShaderCalls.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
/// @brief An App on @p mode with a script host on it: on a client, the ShaderPlugin over the fake backend, which runs
/// game shaders; on a server, only the shader plugin's setup forms, as its library registers them on every side.
struct Scripts {
    std::shared_ptr<aylo::test::plugins::FakeBackendState> state{
        std::make_shared<aylo::test::plugins::FakeBackendState>()};
    aylo::ecs::App app;
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    explicit Scripts(aylo::ecs::RunMode mode) : app{mode} {
        state->shaders = true;
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{100}));
        const bool client = mode == aylo::ecs::RunMode::kClient;
        if (client) {
            app.addPlugin(aylo::plugins::transform::TransformPlugin{});
            app.addPlugin(aylo::plugins::input::InputPlugin{});
            app.addPlugin(aylo::plugins::graphics::GraphicsPlugin{aylo::test::plugins::makeFakeRegistry(state),
                                                                  aylo::test::plugins::makeFakeSettings()});
            app.addPlugin(aylo::plugins::shader::ShaderPlugin{});
        } else {
            aylo::plugins::shader::addShaderForms(app.getPluginRegistry());
        }
        auto created = aylo::scripting::ScriptHost::create(app, {.isClient = client});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("screen-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};

/// @brief The manual's first example: the engine's bloom, then its tonemap.
constexpr const char* kBloomAndTonemap = R"(
    local bloom = engine.assets.material({
        name = "game.bloom",
        shader = engine.assets.shader("@aylo/bloom.glsl"),
        values = { threshold = 0.8, intensity = 0.5 },
    })
    local tonemap = engine.assets.material({
        name = "game.tonemap",
        shader = engine.assets.shader("@aylo/tonemap.glsl"),
        values = { mode = 2, exposure = 1.2 },
    })
    engine.screenEffects({ bloom, tonemap })
)";
}  // namespace

TEST(ScriptScreenEffects, AScriptRunsTheEnginesBloomAndTonemap) {
    Scripts client{aylo::ecs::RunMode::kClient};
    ASSERT_EQ(client.run(kBloomAndTonemap), "");
    client.app.runFrame();
    ASSERT_FALSE(client.state->screenMaterials.empty());
    EXPECT_EQ(client.state->screenMaterials.back().size(), 2U);
    ASSERT_EQ(client.state->createdShaders.size(), 2U);
    EXPECT_EQ(client.state->createdShaders.front().variants.front().target,
              aylo::contract::renderer::ShaderTarget::kScreen);
}

TEST(ScriptScreenEffects, ASystemChangesTheEffectsThroughTheCall) {
    Scripts client{aylo::ecs::RunMode::kClient};
    ASSERT_EQ(client.run(std::string{kBloomAndTonemap} + R"(
        local cleared = false
        engine.ecs.system("game.Calm", {
            phase = "Update",
            side = "client",
            query = {},
            calls = { "aylo.shader.setScreenMaterials" },
            run = function()
                if cleared then return end
                cleared = true
                engine.call("aylo.shader.setScreenMaterials", { materials = {} })
            end,
        })
    )"),
              "");
    client.app.runFrame();
    ASSERT_FALSE(client.state->screenMaterials.empty());
    EXPECT_TRUE(client.state->screenMaterials.back().empty());
}

TEST(ScriptScreenEffects, AnUnknownMaterialIsAnError) {
    Scripts client{aylo::ecs::RunMode::kClient};
    const std::string error = client.run(R"(engine.screenEffects({ engine.assetId("game.nothing") }))");
    EXPECT_NE(error.find("engine.screenEffects: engine call 'aylo.shader.setScreenMaterials'"), std::string::npos)
        << error;
    EXPECT_NE(error.find("No material"), std::string::npos) << error;
}

TEST(ScriptScreenEffects, AServerIgnoresTheEffects) {
    Scripts server{aylo::ecs::RunMode::kServer};
    EXPECT_EQ(server.run(kBloomAndTonemap), "");
}
