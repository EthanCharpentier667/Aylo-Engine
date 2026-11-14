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
