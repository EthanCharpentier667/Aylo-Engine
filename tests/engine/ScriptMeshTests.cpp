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
#include "TestGltf.hpp"
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

TEST(ScriptMesh, TheMeshFormNamesItsMistake) {
    ClientScripts client;
    EXPECT_NE(client.run("engine.assets.mesh(42)").find("engine.assets.mesh: expected the file's path first"),
              std::string::npos);
}
