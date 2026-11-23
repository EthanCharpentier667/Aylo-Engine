/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptScreenEffectsTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <utility>

#include "FakeBackend.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/AssetDirectory.hpp"
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

    /// @param assets The game's assets on disk, read by the shader plugin; empty: none.
    explicit Scripts(aylo::ecs::RunMode mode, const std::filesystem::path& assets = {}) : app{mode} {
        state->shaders = true;
        if (!assets.empty()) {
            app.insertResource(aylo::plugins::AssetDirectory{.path = assets});
        }
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

TEST(ScriptScreenEffects, AScriptLoadsAMaterialFile) {
    const std::filesystem::path assets =
        std::filesystem::temp_directory_path() / std::format("aylo_material_script_{:08x}", std::random_device{}());
    std::filesystem::create_directories(assets / "materials");
    std::ofstream{assets / "materials/tone.material"}
        << R"({ "shader": "@aylo/tonemap.glsl", "values": { "mode": 2, "exposure": 1.5 } })";
    {
        Scripts client{aylo::ecs::RunMode::kClient, assets};
        ASSERT_EQ(client.run(R"(
            local tone = engine.assets.material("materials/tone.material")
            assert(tone == engine.assetId("materials/tone.material"), "the id of its path")
            engine.screenEffects({ tone })
        )"),
                  "");
        client.app.runFrame();
        ASSERT_FALSE(client.state->screenMaterials.empty());
        EXPECT_EQ(client.state->screenMaterials.back().size(), 1U);
        ASSERT_EQ(client.state->createdMaterials.size(), 1U);

        const std::string error = client.run(R"(engine.assets.material("materials/missing.material"))");
        EXPECT_TRUE(error.contains("materials/missing.material")) << error;
    }
    std::filesystem::remove_all(assets);
}

TEST(ScriptScreenEffects, OnAServerAMaterialFileOnlyGivesItsId) {
    Scripts server{aylo::ecs::RunMode::kServer};
    EXPECT_EQ(server.run(R"(
        assert(engine.assets.material("materials/tone.material") == engine.assetId("materials/tone.material"))
    )"),
              "");
}

TEST(ScriptScreenEffects, AScriptFogsThe3DWorldByItsDepth) {
    // manual/pages/post-processing.md, "Depth and the 3D camera".
    const std::filesystem::path assets =
        std::filesystem::temp_directory_path() / std::format("aylo_fog_script_{:08x}", std::random_device{}());
    std::filesystem::create_directories(assets / "shaders");
    std::ofstream{assets / "shaders/fog.glsl"} << R"(#pragma screen
uniform vec3 fogColor = vec3(0.5, 0.6, 0.7);
uniform float density = 0.04;
void fragment() {
    float distance = linearDepth(texture(DEPTH_TEXTURE, SCREEN_UV).r);
    COLOR.rgb = mix(fogColor, COLOR.rgb, exp(-density * distance));
}
)";
    {
        Scripts client{aylo::ecs::RunMode::kClient, assets};
        ASSERT_EQ(client.run(R"(
            local fog = engine.assets.material({ name = "fog", shader = engine.assets.shader("shaders/fog.glsl"),
                                                 values = { density = 0.08 } })
            engine.screenEffects({ fog })
        )"),
                  "");
        client.app.runFrame();
        ASSERT_EQ(client.state->createdShaders.size(), 1U);
        EXPECT_TRUE(client.state->createdShaders.front().readsDepth);
        ASSERT_FALSE(client.state->screenMaterials.empty());
        EXPECT_EQ(client.state->screenMaterials.back().size(), 1U);
    }
    std::filesystem::remove_all(assets);
}

TEST(ScriptScreenEffects, AScriptBlursAtHalfSizeAndMixesTheGlowIn) {
    // manual/pages/post-processing.md, "Several passes": the issue's three-pass half-size blur, mixed into the picture.
    const std::filesystem::path assets =
        std::filesystem::temp_directory_path() / std::format("aylo_passes_script_{:08x}", std::random_device{}());
    std::filesystem::create_directories(assets / "shaders");
    std::ofstream{assets / "shaders/bright.glsl"} << R"(#pragma screen
uniform float threshold = 0.8;
void fragment() { COLOR.rgb = max(COLOR.rgb - threshold, 0.0); }
)";
    std::ofstream{assets / "shaders/blur.glsl"} << R"(#pragma screen
uniform vec2 direction = vec2(1.0, 0.0);
void fragment() {
    vec2 step = direction * SCREEN_PIXEL_SIZE;
    COLOR.rgb = (texture(TEXTURE, SCREEN_UV - step).rgb + COLOR.rgb + texture(TEXTURE, SCREEN_UV + step).rgb) / 3.0;
}
)";
    std::ofstream{assets / "shaders/composite.glsl"} << R"(#pragma screen
uniform sampler2D glow;
uniform float strength = 0.4;
void fragment() { COLOR.rgb = texture(SCENE_TEXTURE, SCREEN_UV).rgb + texture(glow, SCREEN_UV).rgb * strength; }
)";
    {
        Scripts client{aylo::ecs::RunMode::kClient, assets};
        ASSERT_EQ(client.run(R"(
            local function material(name, shader, values)
                return engine.assets.material({ name = name, shader = engine.assets.shader(shader), values = values })
            end
            local brightPass = material("bright", "shaders/bright.glsl")
            local blurX = material("blurX", "shaders/blur.glsl", { direction = { x = 1, y = 0 } })
            local blurY = material("blurY", "shaders/blur.glsl", { direction = { x = 0, y = 1 } })
            local composite = material("composite", "shaders/composite.glsl")
            engine.screenEffects({
                { material = brightPass, output = "bright", scale = 0.5 },
                { material = blurX, input = "bright", output = "blurA", scale = 0.5 },
                { material = blurY, input = "blurA", output = "blurB", scale = 0.5, lowPrecision = true },
                { material = composite, inputs = { glow = "blurB" } },
            })
        )"),
                  "");
        client.app.runFrame();
        ASSERT_FALSE(client.state->screenEffects.empty());
        const auto& passes = client.state->screenEffects.back();
        ASSERT_EQ(passes.size(), 4U);
        EXPECT_EQ(passes.at(1).input, "bright");
        EXPECT_TRUE(passes.at(2).lowPrecision);
        ASSERT_EQ(passes.at(3).inputs.size(), 1U);
        EXPECT_EQ(passes.at(3).inputs.front().target, "blurB");

        // Back to a list of materials: a chain at full size again.
        ASSERT_EQ(client.run(R"(engine.screenEffects({ engine.assetId("bright") }))"), "");
        client.app.runFrame();
        ASSERT_FALSE(client.state->screenMaterials.empty());
        EXPECT_EQ(client.state->screenMaterials.back().size(), 1U);
    }
    std::filesystem::remove_all(assets);
}

TEST(ScriptScreenEffects, APassReadingALaterTargetIsAnError) {
    Scripts client{aylo::ecs::RunMode::kClient};
    const std::string error = client.run(R"(
        local tonemap = engine.assets.material({ name = "t", shader = engine.assets.shader("@aylo/tonemap.glsl") })
        engine.screenEffects({ { material = tonemap, input = "blur" }, { material = tonemap, output = "blur" } })
    )");
    EXPECT_TRUE(error.contains("pass 1 reads 'blur', which no pass before it writes")) << error;
    const std::string field = client.run(R"(
        local tonemap = engine.assets.material({ name = "t2", shader = engine.assets.shader("@aylo/tonemap.glsl") })
        engine.screenEffects({ { material = tonemap, scale = "half" } })
    )");
    EXPECT_TRUE(field.contains("engine.screenEffects: pass 1: 'scale' must be a number")) << field;
}

TEST(ScriptScreenEffects, AServerIgnoresThePasses) {
    Scripts server{aylo::ecs::RunMode::kServer};
    EXPECT_EQ(server.run(R"(
        local tonemap = engine.assets.material({ name = "t", shader = engine.assets.shader("@aylo/tonemap.glsl") })
        engine.screenEffects({ { material = tonemap, output = "half", scale = 0.5 }, { material = tonemap } })
    )"),
              "");
}
