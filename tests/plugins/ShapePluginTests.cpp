/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShapePluginTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <utility>
#include <vector>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Shape.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/world/World.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shape/ShapeRenderer.hpp"
#include "plugins/sprite/SpriteRenderer.hpp"
#include "plugins/transform/Transform.hpp"

namespace {
/// @brief A client App with every drawing plugin, the ShapePlugin included, over the fake platform and renderer.
class ShapeApp {
  public:
    ShapeApp() : _state{std::make_shared<aylo::test::plugins::FakeBackendState>()}, _app{aylo::ecs::RunMode::kClient} {
        aylo::test::plugins::addDrawingPlugins(_app, aylo::test::plugins::makeFakeRegistry(_state),
                                               aylo::test::plugins::makeFakeSettings());
    }
    ~ShapeApp() = default;
    ShapeApp(const ShapeApp& other) = delete;
    ShapeApp& operator=(const ShapeApp& other) = delete;
    ShapeApp(ShapeApp&& other) = delete;
    ShapeApp& operator=(ShapeApp&& other) = delete;

    [[nodiscard]] aylo::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] aylo::test::plugins::FakeBackendState& state() noexcept { return *_state; }

  private:
    std::shared_ptr<aylo::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    aylo::ecs::App _app;                                            ///< The App under test.
};
}  // namespace

TEST(ShapePlugin, RegistersShapeRenderer) {
    ShapeApp app;
    EXPECT_TRUE(app.app().getWorld().getComponents().find("aylo.ShapeRenderer"));
}

TEST(ShapePlugin, DrawsTheShapesAtTheirGlobalTransformOverTheSprites) {
    ShapeApp app;
    aylo::ecs::World& world = app.app().getWorld();
    const aylo::core::asset::AssetId ship = world.getResources().getMut<aylo::plugins::graphics::TextureAssets>()->load(
        "ship.png", aylo::test::plugins::twoPixelPng());
    const aylo::ecs::Entity sprite = world.spawn();
    world.getComponents().add(sprite, aylo::plugins::transform::Transform{});
    world.getComponents().add(sprite, aylo::plugins::sprite::SpriteRenderer{.texture = ship, .layer = 100});
    const aylo::ecs::Entity wall = world.spawn();
    world.getComponents().add(
        wall, aylo::plugins::transform::Transform{.position = {10.0F, 20.0F}, .rotation = 0.5F, .scale = {2.0F, 3.0F}});
    world.getComponents().add(wall, aylo::plugins::shape::ShapeRenderer{.size = {4.0F, 5.0F},
                                                                        .cornerRadius = 1.0F,
                                                                        .fillColor = {0.5F, 0.5F, 0.5F, 1.0F},
                                                                        .outlineThickness = 2.0F});
    app.state().drawOrder.clear();
    app.app().runFrame();
    EXPECT_EQ(app.state().drawOrder, (std::vector<char>{'s', 'h'}));  // a shape is an overlay, whatever the layers
    ASSERT_EQ(app.state().shapes.size(), 1U);
    const aylo::contract::renderer::Shape& shape = app.state().shapes.at(0);
    EXPECT_EQ(shape.kind, aylo::contract::renderer::ShapeKind::kRect);
    EXPECT_EQ(shape.position, (glm::vec2{10.0F, 20.0F}));
    EXPECT_FLOAT_EQ(shape.rotation, 0.5F);
    EXPECT_EQ(shape.size, (glm::vec2{8.0F, 15.0F}));  // the size scales on both axes...
    EXPECT_FLOAT_EQ(shape.cornerRadius, 2.0F);        // ... the radii and the outline on X
    EXPECT_FLOAT_EQ(shape.outlineThickness, 4.0F);
    EXPECT_EQ(shape.fillColor, (aylo::contract::renderer::Color{0.5F, 0.5F, 0.5F, 1.0F}));
}

TEST(ShapePlugin, DrawsALineToItsEndInTheEntitysSpace) {
    ShapeApp app;
    aylo::ecs::World& world = app.app().getWorld();
    const aylo::ecs::Entity line = world.spawn();
    world.getComponents().add(line, aylo::plugins::transform::Transform{
                                        .position = {10.0F, 0.0F}, .rotation = 1.5707964F, .scale = {2.0F, 2.0F}});
    world.getComponents().add(
        line, aylo::plugins::shape::ShapeRenderer{
                  .kind = aylo::contract::renderer::ShapeKind::kLine, .end = {5.0F, 0.0F}, .radius = 1.0F});
    app.app().runFrame();
    ASSERT_EQ(app.state().shapes.size(), 1U);
    const aylo::contract::renderer::Shape& shape = app.state().shapes.at(0);
    EXPECT_EQ(shape.position, (glm::vec2{10.0F, 0.0F}));
    EXPECT_NEAR(shape.end.x, 10.0F, 1e-4F);  // turned a quarter clockwise on screen: +X becomes +Y (down)
    EXPECT_NEAR(shape.end.y, 10.0F, 1e-4F);
    EXPECT_FLOAT_EQ(shape.radius, 2.0F);
}

TEST(ShapePlugin, DrawsTheShapesByLayer) {
    ShapeApp app;
    aylo::ecs::World& world = app.app().getWorld();
    for (const auto& [x, layer] : {std::pair{3.0F, 5}, std::pair{1.0F, -2}, std::pair{2.0F, 0}}) {
        const aylo::ecs::Entity entity = world.spawn();
        world.getComponents().add(entity, aylo::plugins::transform::Transform{.position = {x, 0.0F}});
        world.getComponents().add(entity, aylo::plugins::shape::ShapeRenderer{.size = {1.0F, 1.0F}, .layer = layer});
    }
    app.app().runFrame();
    ASSERT_EQ(app.state().shapes.size(), 3U);
    EXPECT_FLOAT_EQ(app.state().shapes[0].position.x, 1.0F);
    EXPECT_FLOAT_EQ(app.state().shapes[1].position.x, 2.0F);
    EXPECT_FLOAT_EQ(app.state().shapes[2].position.x, 3.0F);
}
