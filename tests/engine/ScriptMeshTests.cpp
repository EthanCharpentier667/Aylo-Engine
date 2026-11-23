/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptMeshTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "FakeBackend.hpp"
#include "TestModels.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/AssetDirectory.hpp"
#include "plugins/mesh/MeshPlugin.hpp"

namespace {
/// @brief A client App with the drawing plugins and the MeshPlugin over the fake backend, a script host on it, and an
/// assets folder holding the test triangle.
struct ClientScripts {
    aylo::test::plugins::AssetsFolder assets;
    std::shared_ptr<aylo::test::plugins::FakeBackendState> state =
        std::make_shared<aylo::test::plugins::FakeBackendState>();
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    ClientScripts() {
        assets.writeTriangle();
        app.insertResource(aylo::plugins::AssetDirectory{.path = assets.root()});
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{100}));
        aylo::test::plugins::addDrawingPlugins(app, aylo::test::plugins::makeFakeRegistry(state),
                                               aylo::test::plugins::makeFakeSettings());
        app.addPlugin(aylo::plugins::mesh::MeshPlugin{});
        auto created = aylo::scripting::ScriptHost::create(app, {.isClient = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("mesh-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptMesh, AScriptLoadsAModelAndDrawsIt) {
    ClientScripts client;
    ASSERT_EQ(client.run(R"(
        local triangle = engine.assets.mesh("models/tri.gltf")
        engine.ecs.system("game.Setup", {
            phase = "Startup",
            side = "client",
            run = function(ctx)
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(0, 0, -3) },
                    ["aylo.MeshRenderer"] = { mesh = triangle, color = { x = 1, y = 0, z = 0, w = 1 } },
                })
            end,
        })
    )"),
              "");

    for (int frame = 0; frame < 4; ++frame) {
        client.app.runFrame();
    }

    ASSERT_EQ(client.state->createdStaticMeshes.size(), 1U);
    ASSERT_FALSE(client.state->staticMeshDraws.empty());
    EXPECT_FLOAT_EQ(client.state->staticMeshDraws.back().world[3][2], -3.0F);
    EXPECT_FLOAT_EQ(client.state->staticMeshDraws.back().color.x, 1.0F);
}

TEST(ScriptMesh, AScriptLoadsAnObjModelAsAGltfOne) {
    ClientScripts client;
    client.assets.writeQuad();
    ASSERT_EQ(client.run(R"(
        local quad = engine.assets.mesh("models/quad.obj")
        engine.ecs.system("game.Setup", {
            phase = "Startup",
            side = "client",
            run = function(ctx)
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = {},
                    ["aylo.MeshRenderer"] = { mesh = quad },
                })
            end,
        })
    )"),
              "");

    for (int frame = 0; frame < 4; ++frame) {
        client.app.runFrame();
    }

    ASSERT_EQ(client.state->createdStaticMeshes.size(), 1U);
    EXPECT_EQ(client.state->createdStaticMeshes.at(0).indices.size(), 6U);
    EXPECT_FALSE(client.state->staticMeshDraws.empty());
}

TEST(ScriptMesh, TheMeshFormNamesItsMistake) {
    ClientScripts client;
    EXPECT_NE(client.run("engine.assets.mesh(42)").find("engine.assets.mesh: expected the file's path first"),
              std::string::npos);
}

TEST(ScriptMesh, AScriptGivesEachModelItsOwnInstanceValues) {
    // manual/pages/materials.md: "One value per model".
    ClientScripts client;
    client.state->shaders = true;
    client.assets.write("models/burn.glsl",
                        "#pragma mesh\ninstance uniform float dissolve = 0.0;\n"
                        "void fragment() { if (fract(dot(AABB_MAX, vec3(1.0))) < dissolve) { discard; } }\n");
    ASSERT_EQ(client.run(R"(
        local triangle = engine.assets.mesh("models/tri.gltf")
        local burn = engine.assets.material({ name = "burn", shader = engine.assets.shader("models/burn.glsl") })
        local burning = nil
        engine.ecs.system("game.Setup", {
            phase = "Startup",
            side = "client",
            run = function(ctx)
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = {},
                    ["aylo.MeshRenderer"] = { mesh = triangle, material = burn },
                })
                burning = ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(2, 0, 0) },
                    ["aylo.MeshRenderer"] = { mesh = triangle, material = burn },
                })
            end,
        })
        engine.ecs.system("game.Burn", {
            phase = "Update",
            side = "client",
            query = {},
            calls = { "aylo.shader.setInstanceValue" },
            run = function(ctx)
                ctx.call("aylo.shader.setInstanceValue",
                    { entity = burning, material = burn, name = "dissolve", x = 0.3 })
            end,
        })
    )"),
              "");

    for (int frame = 0; frame < 5; ++frame) {
        client.app.runFrame();
    }

    ASSERT_GE(client.state->staticMeshDraws.size(), 2U);
    const auto& draws = client.state->staticMeshDraws;
    const auto& last = draws.back();
    const auto& before = draws.at(draws.size() - 2);
    const auto& moved = last.world[3][0] > 1.0F ? last : before;
    const auto& still = last.world[3][0] > 1.0F ? before : last;
    EXPECT_FLOAT_EQ(moved.instanceValues.at(0).x, 0.3F);
    EXPECT_FLOAT_EQ(still.instanceValues.at(0).x, 0.0F);
}

TEST(ScriptMesh, TheManualsFlashRecipeRuns) {
    // manual/pages/materials.md, "One value per drawing", with the shader in models/ and an entity that has game.Flash.
    ClientScripts client;
    client.state->shaders = true;
    client.assets.write("models/flash.glsl",
                        "instance uniform float amount = 0.0;\n"
                        "void fragment() { COLOR.rgb = mix(COLOR.rgb, vec3(COLOR.a), amount); }\n");
    ASSERT_EQ(client.run(R"(
        local flash = engine.assets.material({ name = "flash", shader = engine.assets.shader("models/flash.glsl") })
        local triangle = engine.assets.mesh("models/tri.gltf")

        engine.ecs.component("game.Flash", { fields = { amount = "f32" } })

        engine.ecs.system("game.Flash", {
            phase = "Update",
            side = "client",
            query = { write = { "game.Flash" } },
            calls = { "aylo.shader.setInstanceValue" },
            run = function(ctx)
                for entity, hit in ctx.query do
                    hit.amount = math.max(hit.amount - ctx.time.delta * 4, 0)
                    ctx.call("aylo.shader.setInstanceValue",
                        { entity = entity, material = flash, name = "amount", x = hit.amount })
                end
            end,
        })

        engine.ecs.system("game.Spawn", {
            phase = "Startup",
            side = "client",
            run = function(ctx)
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = {},
                    ["aylo.MeshRenderer"] = { mesh = triangle, material = flash },
                    ["game.Flash"] = { amount = 1 },
                })
            end,
        })
    )"),
              "");
    testing::internal::CaptureStderr();
    for (int frame = 0; frame < 3; ++frame) {
        client.app.runFrame();
    }
    const std::string log = testing::internal::GetCapturedStderr();
    EXPECT_EQ(log.find("setInstanceValue"), std::string::npos) << log;
}
