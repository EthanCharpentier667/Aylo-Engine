/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PhysicsDebugTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "FakeBackend.hpp"
#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/contract/platform/input/Key.hpp"
#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Lines3D.hpp"
#include "aylo/contract/renderer/drawing/Shape.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/change/Mut.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/Time.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/Collider3D.hpp"
#include "plugins/physics/PhysicsHandle.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/physics/Velocity3D.hpp"
#include "plugins/physics/debug/PhysicsDebug.hpp"
#include "plugins/physics/debug/PhysicsDebugPlugin.hpp"
#include "plugins/physics/debug/PhysicsDebugSettings.hpp"
#include "plugins/physics/debug/PhysicsDebugShapes.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"

using aylo::contract::renderer::Color;
using aylo::contract::renderer::Shape;
using aylo::contract::renderer::ShapeKind;
using aylo::ecs::Entity;
using aylo::plugins::physics::Collider;
using aylo::plugins::physics::ShapeType;
using aylo::plugins::physics::Velocity;
using aylo::plugins::physics::debug::PhysicsDebug;
using aylo::plugins::physics::debug::PhysicsDebugPlugin;
using aylo::plugins::physics::debug::PhysicsDebugSettings;
using aylo::plugins::transform::GlobalTransform2D;
using aylo::plugins::transform::Transform2D;

namespace {
constexpr float kTolerance = 1e-4F;
constexpr Color kRed{1.0F, 0.0F, 0.0F, 1.0F};

/// @brief A client App with the GraphicsPlugin over the fake renderer and the PhysicsDebugPlugin.
struct DebugApp {
    std::shared_ptr<aylo::test::plugins::FakeBackendState> state =
        std::make_shared<aylo::test::plugins::FakeBackendState>();  ///< Drives the fakes, records the shapes.
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};                ///< The App under test.

    DebugApp() {
        aylo::test::plugins::addDrawingPlugins(app, aylo::test::plugins::makeFakeRegistry(state),
                                               aylo::test::plugins::makeFakeSettings());
        app.addPlugin(PhysicsDebugPlugin{});
    }

    [[nodiscard]] aylo::ecs::Components& components() { return app.getWorld().getComponents(); }
    [[nodiscard]] aylo::ecs::Mut<PhysicsDebugSettings> settings() {
        return app.getWorld().getResources().requireMut<PhysicsDebugSettings>();
    }

    /// @return A new entity at the position with a box collider, and the debug component when given.
    Entity spawn(glm::vec2 position, std::optional<PhysicsDebug> debug) {
        const Entity entity = app.getWorld().spawn();
        components().add(entity, Transform2D{.position = position});
        components().add(entity, Collider{.shape = ShapeType::kBox, .size = {40.0F, 20.0F}});
        if (debug) {
            components().add(entity, *debug);
        }
        return entity;
    }

    /// @return The shapes drawn by one more frame.
    std::vector<Shape> frame() {
        state->shapes.clear();
        app.runFrame();
        return state->shapes;
    }
};
}  // namespace

TEST(PhysicsDebugShapes, ABoxOutlineTurnsAndMovesWithItsEntity) {
    const float quarterTurn = std::acos(-1.0F) / 2.0F;
    const Shape shape = aylo::plugins::physics::debug::debugColliderShape(
        Collider{.shape = ShapeType::kBox, .size = {40.0F, 20.0F}, .offset = {10.0F, 0.0F}},
        GlobalTransform2D{.position = {100.0F, 50.0F}, .rotation = quarterTurn}, kRed, 2.0F);
    EXPECT_EQ(shape.kind, ShapeKind::kRect);
    EXPECT_EQ(shape.size, (glm::vec2{40.0F, 20.0F}));
    EXPECT_FLOAT_EQ(shape.rotation, quarterTurn);
    // The offset turns with the entity: a quarter turn clockwise takes +X to +Y (down).
    EXPECT_NEAR(shape.position.x, 100.0F, kTolerance);
    EXPECT_NEAR(shape.position.y, 60.0F, kTolerance);
    EXPECT_FLOAT_EQ(shape.fillColor.w, 0.0F);  // Outline only.
    EXPECT_EQ(shape.outlineColor, kRed);
    EXPECT_FLOAT_EQ(shape.outlineThickness, 2.0F);
}

TEST(PhysicsDebugShapes, CirclesAndCapsulesKeepTheirSizes) {
    const Shape circle = aylo::plugins::physics::debug::debugColliderShape(
        Collider{.shape = ShapeType::kCircle, .radius = 8.0F}, GlobalTransform2D{}, kRed, 1.0F);
    EXPECT_EQ(circle.kind, ShapeKind::kCircle);
    EXPECT_FLOAT_EQ(circle.radius, 8.0F);

    const Shape capsule = aylo::plugins::physics::debug::debugColliderShape(
        Collider{.shape = ShapeType::kCapsule, .radius = 6.0F, .halfHeight = 12.0F}, GlobalTransform2D{}, kRed, 1.0F);
    EXPECT_EQ(capsule.kind, ShapeKind::kCapsule);
    EXPECT_FLOAT_EQ(capsule.radius, 6.0F);
    EXPECT_FLOAT_EQ(capsule.halfHeight, 12.0F);
}

TEST(PhysicsDebugShapes, ASensorIsDrawnFainter) {
    const Shape shape =
        aylo::plugins::physics::debug::debugColliderShape(Collider{.sensor = true}, GlobalTransform2D{}, kRed, 1.0F);
    EXPECT_FLOAT_EQ(shape.outlineColor.w, 0.5F);
}

TEST(PhysicsDebugShapes, AVelocityArrowIsAShaftAndTwoHeadStrokes) {
    const std::vector<Shape> arrow =
        aylo::plugins::physics::debug::debugVelocityArrow({0.0F, 0.0F}, {300.0F, 0.0F}, 0.1F, kRed, 2.0F);
    ASSERT_EQ(arrow.size(), 3U);
    for (const Shape& stroke : arrow) {
        EXPECT_EQ(stroke.kind, ShapeKind::kLine);
        EXPECT_FLOAT_EQ(stroke.radius, 1.0F);  // Half the thickness.
        EXPECT_EQ(stroke.fillColor, kRed);
    }
    EXPECT_EQ(arrow.at(0).end, (glm::vec2{30.0F, 0.0F}));  // 0.1 s of travel at 300 units/s.
    // The head strokes go back from the tip, one on each side.
    EXPECT_LT(arrow.at(1).end.x, 30.0F);
    EXPECT_LT(arrow.at(2).end.x, 30.0F);
    EXPECT_NEAR(arrow.at(1).end.y, -arrow.at(2).end.y, kTolerance);
    EXPECT_GT(std::abs(arrow.at(1).end.y), 0.0F);
}

TEST(PhysicsDebugShapes, AStillBodyHasNoArrow) {
    EXPECT_TRUE(
        aylo::plugins::physics::debug::debugVelocityArrow({5.0F, 5.0F}, {0.0F, 0.0F}, 0.1F, kRed, 1.0F).empty());
}

TEST(PhysicsDebugShapes, ABoxWireframeHasTwelveEdgesPlacedByItsEntity) {
    using aylo::plugins::physics::Collider3D;
    const aylo::contract::renderer::Lines3D lines = aylo::plugins::physics::debug::debugCollider3DLines(
        Collider3D{.size = {2.0F, 4.0F, 6.0F}, .offset = {1.0F, 0.0F, 0.0F}},
        aylo::plugins::transform::GlobalTransform3D{.position = {10.0F, 0.0F, 0.0F}, .scale = {3.0F, 3.0F, 3.0F}},
        kRed);
    ASSERT_EQ(lines.points.size(), 24U);
    for (const glm::vec3& point : lines.points) {
        EXPECT_NEAR(std::abs(point.x - 1.0F), 1.0F, kTolerance);  // around the offset, half the width
        EXPECT_NEAR(std::abs(point.y), 2.0F, kTolerance);
        EXPECT_NEAR(std::abs(point.z), 3.0F, kTolerance);
    }
    // Placed by the entity's pose, not its scale: colliders are not scaled.
    EXPECT_FLOAT_EQ(lines.world[3][0], 10.0F);
    EXPECT_FLOAT_EQ(lines.world[0][0], 1.0F);
    EXPECT_EQ(lines.color, kRed);
}

TEST(PhysicsDebugShapes, SpheresAndCapsulesAreCirclesOfTheirRadius) {
    using aylo::plugins::physics::Collider3D;
    using aylo::plugins::physics::Shape3DType;
    const aylo::contract::renderer::Lines3D sphere = aylo::plugins::physics::debug::debugCollider3DLines(
        Collider3D{.shape = Shape3DType::kSphere, .radius = 2.0F}, aylo::plugins::transform::GlobalTransform3D{}, kRed);
    ASSERT_FALSE(sphere.points.empty());
    for (const glm::vec3& point : sphere.points) {
        EXPECT_NEAR(glm::length(point), 2.0F, kTolerance);
    }
    const aylo::contract::renderer::Lines3D capsule = aylo::plugins::physics::debug::debugCollider3DLines(
        Collider3D{.shape = Shape3DType::kCapsule, .radius = 0.5F, .halfHeight = 1.0F, .sensor = true},
        aylo::plugins::transform::GlobalTransform3D{}, kRed);
    float highest = 0.0F;
    for (const glm::vec3& point : capsule.points) {
        highest = std::max(highest, point.y);
    }
    EXPECT_NEAR(highest, 1.5F, kTolerance);  // the half height and the cap's radius
    EXPECT_FLOAT_EQ(capsule.color.w, 0.5F);  // a sensor is fainter
}

TEST(PhysicsDebugShapes, A3DVelocityArrowIsAShaftAndTwoHeadStrokes) {
    const aylo::contract::renderer::Lines3D arrow =
        aylo::plugins::physics::debug::debugVelocityArrow3D({1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -5.0F}, 0.1F, kRed);
    ASSERT_EQ(arrow.points.size(), 6U);
    EXPECT_EQ(arrow.points.at(1), (glm::vec3{1.0F, 0.0F, -0.5F}));
    EXPECT_GT(arrow.points.at(3).z, -0.5F);  // the strokes go back from the tip
    EXPECT_TRUE(aylo::plugins::physics::debug::debugVelocityArrow3D({}, {}, 0.1F, kRed).points.empty());
}

TEST(PhysicsDebugPlugin, Draws3DCollidersAsLinesWhenTheRendererDrawsThem) {
    DebugApp fixture;
    fixture.settings()->drawAll = true;
    const Entity entity = fixture.app.getWorld().spawn();
    fixture.components().add(entity, aylo::plugins::transform::Transform3D{});
    fixture.components().add(entity, aylo::plugins::physics::Collider3D{});
    fixture.components().add(entity, aylo::plugins::physics::Velocity3D{.linear = {0.0F, 2.0F, 0.0F}});

    fixture.frame();
    fixture.state->lines3D.clear();
    fixture.frame();
    EXPECT_EQ(fixture.state->lines3D.size(), 2U);  // the box and the arrow

    fixture.state->draws3D = false;
    fixture.state->lines3D.clear();
    fixture.frame();
    EXPECT_TRUE(fixture.state->lines3D.empty());
}

TEST(PhysicsDebugPlugin, RegistersItsComponentAndSettings) {
    DebugApp fixture;
    EXPECT_TRUE(fixture.components().find("aylo.PhysicsDebug"));
    EXPECT_TRUE(fixture.app.getWorld().getResources().has<PhysicsDebugSettings>());
}

TEST(PhysicsDebugPlugin, DrawsOnlyTheEntitiesWithPhysicsDebug) {
    DebugApp fixture;
    fixture.spawn({100.0F, 50.0F}, PhysicsDebug{.color = kRed});
    fixture.spawn({300.0F, 50.0F}, std::nullopt);
    const std::vector<Shape> shapes = fixture.frame();
    ASSERT_EQ(shapes.size(), 1U);
    EXPECT_EQ(shapes.at(0).kind, ShapeKind::kRect);
    EXPECT_EQ(shapes.at(0).position, (glm::vec2{100.0F, 50.0F}));
    EXPECT_EQ(shapes.at(0).outlineColor, kRed);
}

TEST(PhysicsDebugPlugin, DrawAllDrawsEveryColliderInTheSettingsColour) {
    DebugApp fixture;
    fixture.settings()->drawAll = true;
    fixture.spawn({300.0F, 50.0F}, std::nullopt);
    const std::vector<Shape> shapes = fixture.frame();
    ASSERT_EQ(shapes.size(), 1U);
    EXPECT_EQ(shapes.at(0).outlineColor, PhysicsDebugSettings{}.color);
}

TEST(PhysicsDebugPlugin, DrawsTheVelocityAsAnArrow) {
    DebugApp fixture;
    const Entity ship = fixture.spawn({0.0F, 0.0F}, PhysicsDebug{.shape = false});
    fixture.components().add(ship, Velocity{.linear = {300.0F, 0.0F}});
    const std::vector<Shape> shapes = fixture.frame();
    ASSERT_EQ(shapes.size(), 3U);  // No outline: only the arrow.
    for (const Shape& stroke : shapes) {
        EXPECT_EQ(stroke.kind, ShapeKind::kLine);
    }
}

TEST(PhysicsDebugPlugin, DrawsNothingWhenDisabled) {
    DebugApp fixture;
    fixture.settings()->enabled = false;
    fixture.spawn({100.0F, 50.0F}, PhysicsDebug{});
    EXPECT_TRUE(fixture.frame().empty());
}

TEST(PhysicsDebugPlugin, TheToggleKeyFlipsTheDrawing) {
    DebugApp fixture;
    fixture.spawn({100.0F, 50.0F}, PhysicsDebug{});
    ASSERT_EQ(fixture.frame().size(), 1U);

    fixture.state->frames.push_back(
        {aylo::contract::platform::event::KeyPressed{.key = aylo::contract::platform::input::Key::kF3}});
    EXPECT_TRUE(fixture.frame().empty());  // F3: off.

    fixture.state->frames.push_back({aylo::contract::platform::event::KeyPressed{
        .key = aylo::contract::platform::input::Key::kF3, .repeat = true}});
    EXPECT_TRUE(fixture.frame().empty());  // An auto-repeat does not flip it back.

    fixture.state->frames.push_back(
        {aylo::contract::platform::event::KeyPressed{.key = aylo::contract::platform::input::Key::kF3}});
    EXPECT_EQ(fixture.frame().size(), 1U);  // F3 again: on.
}

TEST(PhysicsDebugPlugin, AddingPhysicsDebugNeverRebuildsTheBody) {
    aylo::ecs::App app;
    aylo::test::plugins::addDrawingPlugins(
        app, aylo::test::plugins::makeFakeRegistry(std::make_shared<aylo::test::plugins::FakeBackendState>()),
        aylo::test::plugins::makeFakeSettings());
    app.addPlugin(aylo::plugins::transform::TransformPlugin{});
    app.addPlugin(aylo::plugins::physics::PhysicsPlugin{aylo::plugins::physics::PhysicsSettings{
        .maxBodies = 8, .maxBodyPairs = 8, .maxContactConstraints = 8, .tempAllocatorBytes = 1024 * 1024}});
    app.addPlugin(PhysicsDebugPlugin{});
    app.getWorld().getResources().insert(aylo::ecs::Time{.fixedDelta = 1.0F / 60.0F});
    const Entity ship = app.getWorld().spawn();
    app.getWorld().getComponents().add(ship, Transform2D{});
    app.getWorld().getComponents().add(ship, Collider{});
    app.getWorld().getComponents().add(ship, aylo::plugins::physics::RigidBody{});
    app.getSchedules().run(aylo::ecs::Phase::kFixedUpdate);
    const aylo::plugins::physics::PhysicsHandle before =
        *app.getWorld().getComponents().get<aylo::plugins::physics::PhysicsHandle>(ship);

    app.getWorld().getComponents().add(ship, PhysicsDebug{});
    app.getSchedules().run(aylo::ecs::Phase::kFixedUpdate);
    EXPECT_EQ(*app.getWorld().getComponents().get<aylo::plugins::physics::PhysicsHandle>(ship), before);
}
