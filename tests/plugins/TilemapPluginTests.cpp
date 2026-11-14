/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TilemapPluginTests
*/

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <memory>
#include <utility>
#include <vector>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryState.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/ecs/world/World.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/graphics/Camera.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/SpriteQueue.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/graphics/VisibleArea.hpp"
#include "plugins/sprite/SpriteRenderer.hpp"
#include "plugins/tilemap/Parallax.hpp"
#include "plugins/tilemap/SpawnTilemap.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "plugins/tilemap/TilemapLayer.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/Transform.hpp"

using aylo::core::asset::AssetId;
using aylo::plugins::tilemap::LayerKind;
using aylo::plugins::tilemap::MapLayer;
using aylo::plugins::tilemap::Parallax;
using aylo::plugins::tilemap::Tilemap;
using aylo::plugins::tilemap::TilemapAssets;
using aylo::plugins::tilemap::TilemapLayer;
using aylo::plugins::tilemap::Tileset;
using aylo::plugins::transform::GlobalTransform2D;
using aylo::plugins::transform::Transform2D;

namespace {
/// @brief The name the test map is added under.
constexpr const char* kLevel = "level";

/// @brief Startup: spawns the layers of the map "level", their draw layers from 5.
void spawnLevel(aylo::ecs::ResMut<TilemapAssets> maps, aylo::ecs::Commands& commands) {
    static_cast<void>(aylo::plugins::tilemap::spawnTilemap(
        commands, *maps, AssetId::fromPath(kLevel), aylo::plugins::tilemap::TilemapPlacement{.firstDrawLayer = 5}));
}

/// @brief An App with the GraphicsPlugin over the fake backend (an 800x600 window), frames of one second, and the
/// image tiles.png (2x1 pixels: a tileset of two 1x1 tiles) loaded.
class TilemapApp {
  public:
    TilemapApp()
        : _state{std::make_shared<aylo::test::plugins::FakeBackendState>()}, _app{aylo::ecs::RunMode::kClient} {
        aylo::test::plugins::addDrawingPlugins(_app, aylo::test::plugins::makeFakeRegistry(_state),
                                               aylo::test::plugins::makeFakeSettings());
        _app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::seconds{1}));
        static_cast<void>(world().getResources().getMut<aylo::plugins::graphics::TextureAssets>()->load(
            "tiles.png", aylo::test::plugins::twoPixelPng()));
    }
    ~TilemapApp() = default;
    TilemapApp(const TilemapApp& other) = delete;
    TilemapApp& operator=(const TilemapApp& other) = delete;
    TilemapApp(TilemapApp&& other) = delete;
    TilemapApp& operator=(TilemapApp&& other) = delete;

    [[nodiscard]] aylo::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] aylo::ecs::World& world() noexcept { return _app.getWorld(); }
    [[nodiscard]] aylo::test::plugins::FakeBackendState& state() noexcept { return *_state; }

    /// @brief Adds @p layers as the map "level", over a 16x16 grid with the tileset tiles.png, and spawns it.
    void spawnMap(std::vector<MapLayer> layers) {
        const Tileset tiles{.imagePath = "tiles.png",
                            .texture = AssetId::fromPath("tiles.png"),
                            .imageSize = {2, 1},
                            .tileSize = {1, 1},
                            .tileCount = 2,
                            .columns = 2};
        static_cast<void>(
            world().getResources().getMut<TilemapAssets>()->add(kLevel, Tilemap{{16, 16}, {tiles}, std::move(layers)}));
        _app.addSystem(aylo::ecs::Phase::kStartup, "test.spawnLevel", &spawnLevel).setSide(aylo::ecs::Side::kBoth);
    }

    /// @return The sprites drawn by one more frame.
    std::vector<aylo::contract::renderer::Sprite> drawFrame() {
        _state->sprites.clear();
        _app.runFrame();
        return _state->sprites;
    }

  private:
    std::shared_ptr<aylo::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    aylo::ecs::App _app;                                            ///< The App under test.
};

/// @return A tile layer with the tile id 1 in each of @p cells.
MapLayer tilesAt(const std::vector<glm::ivec2>& cells) {
    MapLayer layer{.name = "ground"};
    const std::array<std::uint32_t, 1> tile{1};
    for (const glm::ivec2 cell : cells) {
        aylo::plugins::tilemap::setTiles(layer, cell, 1, tile);
    }
    return layer;
}

/// @return The entities that have a TilemapLayer, with it.
std::vector<std::pair<aylo::ecs::Entity, TilemapLayer>> layersOf(aylo::ecs::World& world) {
    std::vector<std::pair<aylo::ecs::Entity, TilemapLayer>> layers;
    const aylo::ecs::QueryState<const TilemapLayer> state{world.getComponents()};
    aylo::ecs::Query<const TilemapLayer> query{state, 0};
    for (auto [entity, layer] : query) {
        layers.emplace_back(entity, layer);
    }
    return layers;
}
}  // namespace

TEST(TilemapPlugin, ComesWithTheGraphicsPlugin) {
    TilemapApp app;
    EXPECT_TRUE(app.app().hasPlugin(aylo::plugins::tilemap::TilemapPlugin::kName));
    EXPECT_TRUE(app.world().getResources().has<TilemapAssets>());
    EXPECT_TRUE(app.world().getResources().has<aylo::plugins::graphics::SpriteQueue>());
    EXPECT_TRUE(app.world().getResources().has<aylo::plugins::graphics::VisibleArea>());
    EXPECT_TRUE(app.world().getComponents().find("aylo.TilemapLayer"));
    EXPECT_TRUE(app.world().getComponents().find("aylo.Parallax"));
}

TEST(TilemapPlugin, SpawnsOneEntityPerLayer) {
    TilemapApp app;
    MapLayer sky{.name = "sky", .kind = LayerKind::kImage, .offset = {4.0F, 0.0F}, .parallax = {0.5F, 1.0F}};
    app.spawnMap({sky, tilesAt({{0, 0}})});
    app.app().runFrame();
    const auto layers = layersOf(app.world());
    ASSERT_EQ(layers.size(), 2U);
    for (const auto& [entity, layer] : layers) {
        EXPECT_EQ(layer.map, AssetId::fromPath(kLevel));
        EXPECT_EQ(layer.drawLayer, 5 + static_cast<std::int32_t>(layer.layer));
        if (layer.layer == 0) {
            EXPECT_EQ(app.world().getComponents().get<Transform2D>(entity)->position, (glm::vec2{4.0F, 0.0F}));
            EXPECT_EQ(app.world().getComponents().get<Parallax>(entity)->factor, (glm::vec2{0.5F, 1.0F}));
        }
    }
}

TEST(TilemapPlugin, DrawsOnlyTheChunksTheCameraShows) {
    TilemapApp app;
    app.spawnMap({tilesAt({{0, 0}, {1, 0}, {100, 0}})});  // The last one at x 1600, past the 800-pixel view.
    const auto sprites = app.drawFrame();
    ASSERT_EQ(sprites.size(), 2U);
    EXPECT_EQ(sprites.at(0).position, (glm::vec2{0.5F, 15.5F}));  // A 1x1 tile at the bottom-left of its cell.
    EXPECT_EQ(sprites.at(1).position, (glm::vec2{16.5F, 15.5F}));
}

TEST(TilemapPlugin, SkipsAHiddenLayer) {
    TilemapApp app;
    MapLayer hidden = tilesAt({{0, 0}});
    hidden.visible = false;
    app.spawnMap({hidden});
    EXPECT_TRUE(app.drawFrame().empty());
}

TEST(TilemapPlugin, RepeatsAnImageLayerAcrossTheView) {
    TilemapApp app;
    app.spawnMap({MapLayer{.kind = LayerKind::kImage,
                           .imagePath = "tiles.png",
                           .image = AssetId::fromPath("tiles.png"),
                           .repeatX = true}});
    const auto sprites = app.drawFrame();
    ASSERT_EQ(sprites.size(), 1U);
    EXPECT_FLOAT_EQ(sprites.at(0).source.getWidth(), 800.0F);  // 400 copies of the 2-pixel image.
    EXPECT_FLOAT_EQ(sprites.at(0).source.getHeight(), 1.0F);
}

TEST(TilemapPlugin, SortsTheTilesWithTheSpritesByLayer) {
    TilemapApp app;
    app.spawnMap({tilesAt({{0, 0}})});  // Draw layer 5.
    const aylo::ecs::Entity ship = app.world().spawn();
    app.world().getComponents().add(ship, Transform2D{.position = {300.0F, 300.0F}});
    app.world().getComponents().add(
        ship, aylo::plugins::sprite::SpriteRenderer{.texture = AssetId::fromPath("tiles.png"), .layer = 10});
    const auto sprites = app.drawFrame();
    ASSERT_EQ(sprites.size(), 2U);
    EXPECT_EQ(sprites.at(1).position, (glm::vec2{300.0F, 300.0F}));  // Layer 10, on top of the tiles.
}

TEST(TilemapPlugin, MovesAParallaxEntityByTheCamera) {
    TilemapApp app;
    const aylo::ecs::Entity camera = app.world().spawn();
    app.world().getComponents().add(camera, Transform2D{.position = {400.0F, 300.0F}});
    app.world().getComponents().add(camera, aylo::plugins::graphics::Camera2D{});
    const aylo::ecs::Entity far = app.world().spawn();
    app.world().getComponents().add(far, Transform2D{.position = {10.0F, 0.0F}});
    app.world().getComponents().add(far, Parallax{.factor = {0.5F, 0.0F}, .origin = {0.0F, 100.0F}});
    app.app().runFrame();
    // (camera - origin) * (1 - factor): (400, 200) * (0.5, 1).
    EXPECT_EQ(app.world().getComponents().get<GlobalTransform2D>(far)->position, (glm::vec2{210.0F, 200.0F}));
}

TEST(TilemapPlugin, MovesAParallaxChildFromWhereItsParentPutsIt) {
    TilemapApp app;
    const aylo::ecs::Entity camera = app.world().spawn();
    app.world().getComponents().add(camera, Transform2D{.position = {400.0F, 300.0F}});
    app.world().getComponents().add(camera, aylo::plugins::graphics::Camera2D{});
    const aylo::ecs::Entity parent = app.world().spawn();
    app.world().getComponents().add(parent, Transform2D{.position = {100.0F, 0.0F}});
    const aylo::ecs::Entity far = app.world().spawn();
    app.world().getComponents().add(far, Transform2D{.position = {10.0F, 0.0F}});
    app.world().getComponents().add(far, Parallax{.factor = {0.5F, 0.0F}, .origin = {0.0F, 100.0F}});
    EXPECT_TRUE(app.world().setParent(far, parent));
    app.app().runFrame();
    EXPECT_EQ(app.world().getComponents().get<GlobalTransform2D>(far)->position, (glm::vec2{310.0F, 200.0F}));
    app.app().runFrame();  // the shift goes on top of this frame's position: it does not add up
    EXPECT_EQ(app.world().getComponents().get<GlobalTransform2D>(far)->position, (glm::vec2{310.0F, 200.0F}));
    app.world().getComponents().getMut<Transform2D>(parent)->position.x = 200.0F;
    app.app().runFrame();
    EXPECT_EQ(app.world().getComponents().get<GlobalTransform2D>(far)->position, (glm::vec2{410.0F, 200.0F}));
}

TEST(TilemapPlugin, ScrollsAParallaxEntityOnItsOwn) {
    TilemapApp app;
    const aylo::ecs::Entity cloud = app.world().spawn();
    app.world().getComponents().add(cloud, Transform2D{});
    app.world().getComponents().add(cloud, Parallax{.scroll = {-3.0F, 0.0F}});
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_EQ(app.world().getComponents().get<Parallax>(cloud)->scrolled, (glm::vec2{-6.0F, 0.0F}));
    EXPECT_EQ(app.world().getComponents().get<GlobalTransform2D>(cloud)->position, (glm::vec2{-6.0F, 0.0F}));
}
