/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** main
*/

// A side-scrolling level made in Tiled's format (examples/assets/levels/space.tmj): the camera flies right on its own
// over three repeating background layers and two planets, each moving at its own parallax speed, and a long layer of
// tiles that moves with the world. Only the chunks of tiles the camera shows are drawn. At the end of the level, the
// camera goes back to the start.
//   xmake f --tilemap=y && xmake run tilemap

#include <exception>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/MinimalPlugins.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryTerms.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/time/Time.hpp"
#include "core/asset/AssetId.hpp"
#include "core/config/Settings.hpp"
#include "platform/glfw/Registration.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/Camera.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/GraphicsResources.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/tilemap/SpawnTilemap.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "render/vulkan/Registration.hpp"

using namespace aylo;
using namespace aylo::contract::renderer;
using namespace aylo::plugins::graphics;
using namespace aylo::plugins::tilemap;
using aylo::core::asset::AssetId;
using aylo::plugins::transform::Transform2D;

namespace {
// The view: the size of the background images, shown four times bigger in the window.
constexpr glm::vec2 kView{272.0F, 160.0F};
constexpr float kWindowScale = 4.0F;
constexpr aylo::contract::renderer::Color kClear{0.0F, 0.0F, 0.0F, 1.0F};

constexpr float kCameraSpeed = 48.0F;    // World units per second.
constexpr float kLevelLength = 3200.0F;  // The level's width: 200 tiles of 16 pixels.

// Startup: loads the level, spawns its layers and the camera at its start.
void spawnLevel(ecs::ResMut<TilemapAssets> maps, ecs::Commands& commands) {
    const AssetId level = maps->load("examples/assets/levels/space.tmj");
    const std::vector<ecs::Entity> layers = spawnTilemap(commands, *maps, level);
    std::cout << "examples/assets/levels/space.tmj: " << layers.size() << " layers\n";
    commands.spawn().add(Transform2D{.position = kView / 2.0F}).add(aylo::plugins::graphics::Camera2D{});
}

// Update: flies the camera right, back to the start at the end of the level.
void flyCamera(ecs::Res<ecs::Time> time,
               ecs::Query<Transform2D, ecs::With<aylo::plugins::graphics::Camera2D>> cameras) {
    for (auto [entity, transform] : cameras) {
        transform->position.x += kCameraSpeed * time->delta;
        if (transform.get().position.x > kLevelLength - (kView.x / 2.0F)) {
            transform->position.x = kView.x / 2.0F;
        }
    }
}

// The window and the renderer.
core::config::Settings makeSettings() {
    core::config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "Tilemap and parallax");
    settings.set("window.size", std::vector<double>{kView.x * kWindowScale, kView.y * kWindowScale});
    settings.set("vulkan.virtualResolution", std::vector<double>{kView.x, kView.y});
    return settings;
}
}  // namespace

int main() {
    try {
        auto registry = std::make_unique<BackendRegistry>();
        aylo::platform::glfw::registerPlatform(registry->platforms);
        render::vulkan::registerRenderer(registry->renderers);

        ecs::App app;
        app.addPlugins(ecs::MinimalPlugins{})
            .addPlugin(aylo::plugins::transform::TransformPlugin{})
            .addPlugin(aylo::plugins::input::InputPlugin{})
            .addPlugin(GraphicsPlugin{std::move(registry), makeSettings()})
            .addPlugin(aylo::plugins::tilemap::TilemapPlugin{});
        app.insertResource(ClearColor{.value = kClear});
        app.addSystem(ecs::Phase::kStartup, "example.spawnLevel", &spawnLevel);
        app.addSystem(ecs::Phase::kUpdate, "example.flyCamera", &flyCamera);
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "tilemap: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "tilemap: unknown error\n";
    }
    return 1;
}
