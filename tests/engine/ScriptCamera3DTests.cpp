/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptCamera3DTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "FakeBackend.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
/// @brief A client App with the graphics plugin over the fake backend, and a script host on it.
struct ClientScripts {
    std::shared_ptr<aylo::test::plugins::FakeBackendState> state =
        std::make_shared<aylo::test::plugins::FakeBackendState>();
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    ClientScripts() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{100}));
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        app.addPlugin(aylo::plugins::input::InputPlugin{});
        app.addPlugin(aylo::plugins::graphics::GraphicsPlugin{aylo::test::plugins::makeFakeRegistry(state),
                                                              aylo::test::plugins::makeFakeSettings()});
        auto created = aylo::scripting::ScriptHost::create(app, {.isClient = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("camera3d-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptCamera3D, AScriptLooksAtItsLinesThroughA3DCamera) {
    ClientScripts client;
    ASSERT_EQ(client.run(R"(
        engine.ecs.system("game.Setup", {
            phase = "Startup",
            side = "client",
            run = function(ctx)
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = {
                        position = vector.create(0, 2, 6),
                        rotation = engine.math.quat.lookRotation(vector.create(0, -2, -6)),
                    },
                    ["aylo.Camera3D"] = { projection = "orthographic", orthographicHeight = 8 },
                })
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = {},
                    ["aylo.Lines3D"] = {
                        points = { vector.create(-1, 0, 0), vector.create(1, 0, 0) },
                        color = { x = 0, y = 1, z = 0, w = 1 },
                    },
                })
            end,
        })
    )"),
              "");

    client.app.runFrame();
    client.app.runFrame();

    ASSERT_FALSE(client.state->views3D.empty());
    const aylo::contract::renderer::View3D& view = client.state->views3D.back();
    EXPECT_FLOAT_EQ(view.position.z, 6.0F);
    ASSERT_TRUE(std::holds_alternative<aylo::contract::renderer::Orthographic3D>(view.projection));
    EXPECT_FLOAT_EQ(std::get<aylo::contract::renderer::Orthographic3D>(view.projection).height, 8.0F);
    ASSERT_FALSE(client.state->lines3D.empty());
    EXPECT_EQ(client.state->lines3D.back().points.size(), 2U);
}

TEST(ScriptCamera3D, AProjectionNameItDoesNotHaveIsAnError) {
    ClientScripts client;
    ASSERT_EQ(client.run(R"(
        engine.ecs.system("game.Setup", {
            phase = "Startup",
            side = "client",
            run = function(ctx)
                ctx.commands:spawn({ ["aylo.Transform3D"] = {}, ["aylo.Camera3D"] = { projection = "fisheye" } })
            end,
        })
    )"),
              "");

    testing::internal::CaptureStderr();
    client.app.runFrame();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("'fisheye' is not one of perspective, orthographic"), std::string::npos) << log;
}

TEST(ScriptCamera3D, AScriptLightsThe3DWorld) {
    // manual/pages/lights.md: the first example.
    ClientScripts client;
    ASSERT_EQ(client.run(R"(
        engine.ecs.system("game.Lights", {
            phase = "Startup",
            side = "client",
            run = function(ctx)
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(3, 2.4, 0.3) },
                    ["aylo.Light3D"] = { kind = "area", color = vector.create(1, 0.6, 0.38), power = 600,
                                         size = { x = 5, y = 5 } },
                })
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { rotation = engine.math.quat.fromEuler(math.rad(-50), 0, 0) },
                    ["aylo.Light3D"] = { kind = "directional", intensity = 1.4 },
                })
                ctx.commands:spawn({ ["aylo.Environment3D"] = { ambient = vector.create(0.05, 0.05, 0.06) } })
            end,
        })
    )"),
              "");
    client.app.runFrame();
    client.app.runFrame();

    ASSERT_FALSE(client.state->lightFrames.empty());
    const auto& lights = client.state->lightFrames.back();
    ASSERT_EQ(lights.size(), 2U);
    EXPECT_EQ(lights.at(0).kind, aylo::contract::renderer::LightKind3D::kArea);
    EXPECT_FLOAT_EQ(lights.at(0).intensity, 600.0F);
    EXPECT_FLOAT_EQ(lights.at(0).size.x, 5.0F);
    EXPECT_FLOAT_EQ(lights.at(0).position.y, 2.4F);
    EXPECT_FLOAT_EQ(lights.at(1).intensity, 1.4F);
    EXPECT_LT(lights.at(1).direction.y, 0.0F);  // turned down
    EXPECT_FLOAT_EQ(client.state->environments.back().ambient.z, 0.06F);
}

TEST(ScriptCamera3D, AScriptMakesALightCastShadowsAndSetsThem) {
    // manual/pages/lights.md, "Shadows".
    ClientScripts client;
    ASSERT_EQ(client.run(R"(
        engine.ecs.system("game.Lights", {
            phase = "Startup",
            side = "client",
            run = function(ctx)
                ctx.commands:spawn({
                    ["aylo.Transform3D"] = { position = vector.create(0, 4, 0) },
                    ["aylo.Light3D"] = { kind = "spot", power = 800, castShadows = true, shadowSoftness = 2 },
                })
                ctx.commands:spawn({
                    ["aylo.Environment3D"] = { shadowCascades = 4, shadowResolution = 2048, shadowDistance = 80 },
                })
            end,
        })
    )"),
              "");
    client.app.runFrame();
    client.app.runFrame();
    ASSERT_FALSE(client.state->lightFrames.empty());
    const auto& light = client.state->lightFrames.back().at(0);
    EXPECT_TRUE(light.castShadows);
    EXPECT_FLOAT_EQ(light.shadowSoftness, 2.0F);
    EXPECT_EQ(client.state->environments.back().shadowCascades, 4U);
    EXPECT_EQ(client.state->environments.back().shadowResolution, 2048U);
    EXPECT_FLOAT_EQ(client.state->environments.back().shadowDistance, 80.0F);
}
