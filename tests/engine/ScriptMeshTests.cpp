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
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
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

TEST(ScriptMesh, AScriptDrawsAModelWithARimShaderAndAVarying) {
    // manual/pages/materials.md, "3D materials" and "From vertex() to fragment()".
    ClientScripts client;
    client.state->shaders = true;
    client.assets.write("models/rim.glsl",
                        "#pragma mesh\n"
                        "varying vec3 objectPosition;\n"
                        "void vertex() { objectPosition = VERTEX; }\n"
                        "void fragment() {\n"
                        "    float rim = 1.0 - max(dot(NORMAL, VIEW_DIR), 0.0);\n"
                        "    COLOR.rgb += pow(rim, 3.0) * vec3(0.3, 0.6, 1.0) * (FRONT_FACING ? 1.0 : 0.0);\n"
                        "    COLOR.rgb *= 0.5 + 0.5 * fract(objectPosition.y * 4.0);\n"
                        "}\n");
    ASSERT_EQ(client.run(R"(
        local triangle = engine.assets.mesh("models/tri.gltf")
        local rim = engine.assets.material({ name = "rim", shader = engine.assets.shader("models/rim.glsl") })
        engine.ecs.system("game.Setup", {
            phase = "Startup",
            side = "client",
            run = function(ctx)
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = {},
                    ["aylo.MeshRenderer"] = { mesh = triangle, material = rim },
                })
            end,
        })
    )"),
              "");
    testing::internal::CaptureStderr();
    for (int frame = 0; frame < 4; ++frame) {
        client.app.runFrame();
    }
    const std::string log = testing::internal::GetCapturedStderr();
    EXPECT_EQ(log.find("rim.glsl"), std::string::npos) << log;
    ASSERT_FALSE(client.state->staticMeshDraws.empty());
    EXPECT_NE(client.state->staticMeshDraws.back().material, aylo::contract::renderer::MaterialId{});
}

TEST(ScriptMesh, AMaterialChangesItsShadersRenderState) {
    // manual/pages/materials.md, "How a 3D material is drawn".
    ClientScripts client;
    client.state->shaders = true;
    client.assets.write("models/hull.glsl",
                        "#pragma mesh\n#pragma cull front\nvoid vertex() { VERTEX += NORMAL * 0.05; }\n"
                        "void fragment() { COLOR = vec4(0.0, 0.0, 0.0, 1.0); }\n");
    ASSERT_EQ(client.run(R"(
        engine.assets.material({
            name = "glassHull",
            shader = engine.assets.shader("models/hull.glsl"),
            renderState = { blend = "alpha", depthBias = -1 },
        })
    )"),
              "");
    client.app.runFrame();

    ASSERT_FALSE(client.state->createdMaterials.empty());
    const auto& state = client.state->createdMaterials.back().renderState;
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state->cull, aylo::contract::renderer::CullMode::kFront);  // the shader's, kept
    EXPECT_EQ(state->blend, aylo::contract::renderer::MeshBlend::kAlpha);
    EXPECT_EQ(state->depth, aylo::contract::renderer::DepthMode::kTestNoWrite);  // blended: no depth write
    EXPECT_FLOAT_EQ(state->depthBias, -1.0F);

    EXPECT_NE(client
                  .run(R"(
        engine.assets.material({ name = "odd", shader = engine.assets.shader("models/hull.glsl"),
                                 renderState = { cull = "sideways" } })
    )")
                  .find("'sideways' is not a render state cull"),
              std::string::npos);
}

TEST(ScriptMesh, AScriptGivesEachPartItsMaterialAndAMaterialItsNextPass) {
    // manual/pages/materials.md, "Several materials on one model" and "Passes".
    ClientScripts client;
    client.state->shaders = true;
    client.assets.writeTwoParts();
    client.assets.write("models/plain.glsl", "#pragma mesh\nvoid fragment() {}\n");
    ASSERT_EQ(client.run(R"(
        local model = engine.assets.mesh("models/parts.gltf")
        local plain = engine.assets.shader("models/plain.glsl")
        local outline = engine.assets.material({ name = "outline", shader = plain })
        local paint = engine.assets.material({ name = "paint", shader = plain, nextPass = outline })
        local glass = engine.assets.material({ name = "glass", shader = plain })
        local placed = false
        engine.ecs.system("game.Place", {
            phase = "Update",
            side = "client",
            query = {},
            calls = { "aylo.mesh.findPart" },
            run = function(ctx)
                local glassPart = ctx.call("aylo.mesh.findPart", { mesh = model, name = "Glass" })
                if placed or glassPart == 0 then
                    return
                end
                placed = true
                local materials = { paint, paint }
                materials[glassPart] = glass
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(7, 0, 0) },
                    ["aylo.MeshRenderer"] = { mesh = model, material = paint, materials = materials },
                })
            end,
        })
    )"),
              "");
    for (int frame = 0; frame < 6; ++frame) {
        client.app.runFrame();
    }

    const auto& draws = client.state->staticMeshDraws;
    const auto& parts = client.state->staticMeshPartMaterials;
    ASSERT_GE(draws.size(), 2U);
    const std::size_t model = draws.size() - 2;
    EXPECT_FLOAT_EQ(draws.at(model).world[3][0], 7.0F);
    ASSERT_EQ(parts.at(model).size(), 2U);
    EXPECT_NE(parts.at(model).at(0), parts.at(model).at(1));  // paint, then glass
    EXPECT_TRUE(draws.back().listedPartsOnly);                // paint's outline, on the body only
    ASSERT_EQ(parts.back().size(), 2U);
    EXPECT_NE(parts.back().at(0), aylo::contract::renderer::MaterialId{});
    EXPECT_EQ(parts.back().at(1), aylo::contract::renderer::MaterialId{});
}
