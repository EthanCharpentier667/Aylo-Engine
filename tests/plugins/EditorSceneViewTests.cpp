/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorSceneViewTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <optional>
#include <string>

#include "aylo/contract/renderer/view/CameraView.hpp"
#include "aylo/contract/renderer/view/CameraView3D.hpp"
#include "aylo/contract/renderer/view/View.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/world/World.hpp"
#include "editor/scene/Gizmo.hpp"
#include "editor/scene/Isolation.hpp"
#include "editor/scene/Picking.hpp"
#include "editor/scene/SceneCamera.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
namespace scene = aylo::editor::scene;
namespace capsule = aylo::plugins::capsule;
using aylo::plugins::transform::Transform2D;
using aylo::plugins::transform::Transform3D;
using scene::SceneCamera;
using scene::SceneMode;

constexpr float kTolerance = 1e-3F;

/// @return Whether two vectors are equal within kTolerance.
bool near(const glm::vec3& first, const glm::vec3& second) { return glm::length(first - second) < kTolerance; }
}  // namespace

TEST(SceneCamera, LooksAtItsPivotFromItsDistance) {
    const SceneCamera camera{.pivot = {1.0F, 0.0F, 0.0F}, .yaw = 0.0F, .pitch = 0.0F, .distance = 5.0F};
    EXPECT_TRUE(near(camera.forward(), {0.0F, 0.0F, -1.0F}));
    EXPECT_TRUE(near(camera.position(), {1.0F, 0.0F, 5.0F}));
    EXPECT_TRUE(near(camera.view3D().position, camera.position()));
}

TEST(SceneCamera, OrbitingKeepsThePivotAndLookingKeepsThePosition) {
    SceneCamera orbiting{.pivot = {0.0F, 1.0F, 0.0F}};
    orbiting.orbit({100.0F, 30.0F});
    EXPECT_TRUE(near(orbiting.pivot, {0.0F, 1.0F, 0.0F}));
    EXPECT_NEAR(glm::length(orbiting.position() - orbiting.pivot), orbiting.distance, kTolerance);

    SceneCamera looking;
    const glm::vec3 eye = looking.position();
    looking.look({-80.0F, 20.0F});
    EXPECT_TRUE(near(looking.position(), eye));
    EXPECT_LE(std::abs(looking.pitch), 1.55F);
}

TEST(SceneCamera, FliesPansAndComesCloser) {
    SceneCamera camera{.yaw = 0.0F, .pitch = 0.0F};
    camera.fly({0.0F, 0.0F, 1.0F}, 1.0F, false);  // forward, one second
    EXPECT_NEAR(camera.pivot.z, -6.0F, kTolerance);
    camera.fly({0.0F, 0.0F, 1.0F}, 1.0F, true);  // Shift: four times as fast
    EXPECT_NEAR(camera.pivot.z, -30.0F, kTolerance);

    const glm::vec3 before = camera.pivot;
    camera.pan({100.0F, 0.0F}, 500.0F);  // the world follows the mouse: the camera goes left
    EXPECT_LT(camera.pivot.x, before.x);

    const float distance = camera.distance;
    camera.dolly(1.0F);
    EXPECT_LT(camera.distance, distance);
}

TEST(SceneCamera, In2DPansAndZooms) {
    SceneCamera camera{.mode = SceneMode::k2D, .center = {100.0F, 50.0F}, .zoom = 2.0F};
    camera.pan({20.0F, -10.0F}, 600.0F);
    EXPECT_EQ(camera.center, (glm::vec2{90.0F, 55.0F}));
    camera.dolly(1.0F);
    EXPECT_GT(camera.zoom, 2.0F);
    camera.frame({7.0F, 8.0F, 0.0F}, 1.0F);
    EXPECT_EQ(camera.center, (glm::vec2{7.0F, 8.0F}));
    EXPECT_FLOAT_EQ(std::get<aylo::contract::renderer::Orthographic>(camera.view2D(2.0F).projection).zoom,
                    camera.zoom * 2.0F);
}

TEST(SceneCamera, FramingPutsThePivotOnTheTarget) {
    SceneCamera camera;
    camera.frame({3.0F, 2.0F, 1.0F}, 2.0F);
    EXPECT_TRUE(near(camera.pivot, {3.0F, 2.0F, 1.0F}));
    EXPECT_FLOAT_EQ(camera.distance, 6.0F);
}

TEST(Gizmo, GrabsTheNearestHandle) {
    const std::array handles{
        scene::GizmoHandle{.axis = 0, .from = {0.0F, 0.0F}, .to = {80.0F, 0.0F}},
        scene::GizmoHandle{.axis = 1, .from = {0.0F, 0.0F}, .to = {0.0F, -80.0F}},
    };
    EXPECT_EQ(scene::hitGizmo(handles, {40.0F, 3.0F}, 7.0F), std::optional<std::uint8_t>{0});
    EXPECT_EQ(scene::hitGizmo(handles, {2.0F, -50.0F}, 7.0F), std::optional<std::uint8_t>{1});
    EXPECT_FALSE(scene::hitGizmo(handles, {50.0F, 50.0F}, 7.0F));
}

TEST(Gizmo, MovesRotatesAndScalesAlongItsAxis) {
    const scene::GizmoHandle x{.axis = 0, .from = {0.0F, 0.0F}, .to = {100.0F, 0.0F}, .worldLength = 2.0F};
    const Transform3D start{.position = {1.0F, 0.0F, 0.0F}};

    // Half the handle's length on screen is half its length in the world.
    const Transform3D moved = scene::dragGizmo(scene::GizmoTool::kMove, x, start, {50.0F, 30.0F});
    EXPECT_TRUE(near(moved.position, {2.0F, 0.0F, 0.0F}));

    const Transform3D scaled = scene::dragGizmo(scene::GizmoTool::kScale, x, start, {100.0F, 0.0F});
    EXPECT_FLOAT_EQ(scaled.scale.x, 2.0F);
    EXPECT_FLOAT_EQ(scaled.scale.y, 1.0F);

    const Transform3D turned = scene::dragGizmo(scene::GizmoTool::kRotate, x, start, {0.0F, 50.0F});
    EXPECT_NE(turned.rotation.x, 0.0F);  // around X only
    EXPECT_FLOAT_EQ(turned.rotation.y, 0.0F);
    EXPECT_NEAR(glm::length(turned.rotation), 1.0F, kTolerance);
}

TEST(Gizmo, Moves2DEntitiesAlongXOrY) {
    const scene::GizmoHandle y{.axis = 1, .from = {0.0F, 0.0F}, .to = {0.0F, 80.0F}, .worldLength = 40.0F};
    const Transform2D moved = scene::dragGizmo2D(scene::GizmoTool::kMove, y, Transform2D{}, {5.0F, 40.0F});
    EXPECT_FLOAT_EQ(moved.position.x, 0.0F);
    EXPECT_FLOAT_EQ(moved.position.y, 20.0F);
}

namespace {
/// @brief An App with the transform and capsule plugins and a level: a crate (3D) with a lid, and a sign (2D).
class ScenePickingTest : public ::testing::Test {
  protected:
    void SetUp() override {
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        app.addPlugin(capsule::CapsulePlugin{});
        app.finishPlugins();
        capsule::Capsule level{"scenes/level.scene"};
        auto& components = app.getWorld().getComponents();
        const std::uint32_t crate = level.addNode("Crate", 0);
        level.editNode(crate).values.set(components, Transform3D{.position = {0.0F, 0.0F, -10.0F}});
        const std::uint32_t lid = level.addNode("Lid", crate);
        level.editNode(lid).values.set(components, Transform3D{.position = {0.0F, 3.0F, 0.0F}});
        const std::uint32_t sign = level.addNode("Sign", 0);
        level.editNode(sign).values.set(components, Transform2D{.position = {200.0F, 100.0F}});
        app.getWorld().getResources().getMut<capsule::Capsules>()->define(std::move(level));
        std::string error;
        isolation = scene::isolate(app.getWorld(), levelId, error);
        ASSERT_TRUE(isolation.has_value()) << error;
        app.runFrame();  // the global transforms
    }

    [[nodiscard]] const capsule::Capsule& level() const {
        return *app.getWorld().getResources().get<capsule::Capsules>()->find(levelId);
    }

    const capsule::CapsuleId levelId = capsule::CapsuleId::fromPath("scenes/level.scene");
    aylo::ecs::App app;
    std::optional<scene::Isolation> isolation;
};
}  // namespace

TEST_F(ScenePickingTest, FindsTheEntityOfANodeAndBack) {
    const std::optional<aylo::ecs::Entity> lid = scene::entityOfNode(app.getWorld(), level(), 2);
    ASSERT_TRUE(lid);
    EXPECT_EQ(scene::nodeOfEntity(app.getWorld(), level(), *lid), std::optional<std::uint32_t>{2});
    EXPECT_EQ(scene::entityOfNode(app.getWorld(), level(), 0), isolation->root);
    EXPECT_FALSE(scene::entityOfNode(app.getWorld(), level(), 99));
    EXPECT_FALSE(scene::nodeOfEntity(app.getWorld(), level(), isolation->anchor));  // above the capsule
}

TEST_F(ScenePickingTest, PicksThe3DEntityUnderTheMouse) {
    // From the isolated origin's place, looking along -Z at the crate.
    const auto* crate = app.getWorld().getComponents().get<aylo::plugins::transform::GlobalTransform3D>(
        *scene::entityOfNode(app.getWorld(), level(), 1));
    ASSERT_NE(crate, nullptr);
    const aylo::contract::renderer::CameraView3D view{
        aylo::contract::renderer::View3D{.position = crate->position + glm::vec3{0.0F, 0.0F, 10.0F}}, glm::vec2{0.0F},
        glm::uvec2{800, 600}};
    const std::optional<aylo::ecs::Entity> picked = scene::pick3D(app.getWorld(), view, {400.0F, 300.0F});
    ASSERT_TRUE(picked);
    EXPECT_EQ(scene::nodeOfEntity(app.getWorld(), level(), *picked), std::optional<std::uint32_t>{1});
    EXPECT_FALSE(scene::pick3D(app.getWorld(), view, {5.0F, 5.0F}));
}

TEST_F(ScenePickingTest, PicksThe2DEntityNearTheMouse) {
    const auto* sign = app.getWorld().getComponents().get<aylo::plugins::transform::GlobalTransform2D>(
        *scene::entityOfNode(app.getWorld(), level(), 3));
    ASSERT_NE(sign, nullptr);
    const aylo::contract::renderer::CameraView view{aylo::contract::renderer::View{.position = sign->position},
                                                    glm::vec2{0.0F}, glm::uvec2{800, 600}};
    const std::optional<aylo::ecs::Entity> picked = scene::pick2D(app.getWorld(), view, {405.0F, 298.0F}, 16.0F);
    ASSERT_TRUE(picked);
    EXPECT_EQ(scene::nodeOfEntity(app.getWorld(), level(), *picked), std::optional<std::uint32_t>{3});
    EXPECT_FALSE(scene::pick2D(app.getWorld(), view, {600.0F, 100.0F}, 16.0F));
}
