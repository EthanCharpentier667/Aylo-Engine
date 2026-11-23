/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Graphics3DTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "FakeBackend.hpp"
#include "aylo/contract/renderer/view/CameraView3D.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/world/World.hpp"
#include "plugins/graphics/Camera.hpp"
#include "plugins/graphics/Light.hpp"
#include "plugins/graphics/Lines3D.hpp"
#include "plugins/transform/Transform.hpp"

namespace {
using aylo::contract::renderer::CameraView3D;
using aylo::contract::renderer::Orthographic3D;
using aylo::contract::renderer::Perspective;
using aylo::contract::renderer::View3D;
using aylo::plugins::graphics::Camera3D;
using aylo::plugins::graphics::CameraProjection;
using aylo::plugins::graphics::Lines3D;
using aylo::plugins::transform::Transform3D;

/// @brief An App with the drawing plugins over the fake platform and renderer.
class BackendApp {
  public:
    BackendApp()
        : _state{std::make_shared<aylo::test::plugins::FakeBackendState>()}, _app{aylo::ecs::RunMode::kClient} {
        aylo::test::plugins::addDrawingPlugins(_app, aylo::test::plugins::makeFakeRegistry(_state),
                                               aylo::test::plugins::makeFakeSettings());
    }

    [[nodiscard]] aylo::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] aylo::test::plugins::FakeBackendState& state() noexcept { return *_state; }

  private:
    std::shared_ptr<aylo::test::plugins::FakeBackendState> _state;
    aylo::ecs::App _app;
};
}  // namespace

TEST(CameraView3D, TheCentreOfTheScreenLooksForward) {
    const CameraView3D shown{View3D{.position = {0.0F, 1.0F, 5.0F}}, glm::vec2{0.0F}, glm::uvec2{800, 600}};

    const aylo::contract::renderer::Ray3D ray = shown.screenToRay({400.0F, 300.0F});

    EXPECT_NEAR(ray.direction.x, 0.0F, 1e-4F);
    EXPECT_NEAR(ray.direction.y, 0.0F, 1e-4F);
    EXPECT_NEAR(ray.direction.z, -1.0F, 1e-4F);
    EXPECT_NEAR(ray.origin.y, 1.0F, 1e-3F);
}

TEST(CameraView3D, PlacesThePointsInFrontAndHidesThoseBehind) {
    const CameraView3D shown{View3D{}, glm::vec2{0.0F}, glm::uvec2{800, 600}};

    const std::optional<glm::vec2> ahead = shown.worldToScreen({0.0F, 0.0F, -10.0F});
    const std::optional<glm::vec2> above = shown.worldToScreen({0.0F, 1.0F, -10.0F});

    ASSERT_TRUE(ahead && above);
    EXPECT_NEAR(ahead->x, 400.0F, 1e-2F);
    EXPECT_NEAR(ahead->y, 300.0F, 1e-2F);
    EXPECT_LT(above->y, ahead->y);  // up in the world is up on the screen
    EXPECT_FALSE(shown.worldToScreen({0.0F, 0.0F, 10.0F}));
}

TEST(CameraView3D, AnOrthographicViewShowsItsHeight) {
    const CameraView3D shown{View3D{.projection = Orthographic3D{.height = 10.0F}}, glm::vec2{0.0F},
                             glm::uvec2{800, 600}};

    const std::optional<glm::vec2> top = shown.worldToScreen({0.0F, 5.0F, -50.0F});

    ASSERT_TRUE(top);
    EXPECT_NEAR(top->y, 0.0F, 1e-2F);
}

TEST(CameraView3D, GivesTheProjectionInOpenGLsClipSpaceToo) {
    const CameraView3D shown{View3D{.projection = Perspective{.nearPlane = 1.0F, .farPlane = 10.0F}}, glm::vec2{0.0F},
                             glm::uvec2{800, 600}};

    const auto clip = [&shown](const glm::vec3& point) {
        const glm::vec4 projected = shown.getOpenGLProjection() * glm::vec4{point, 1.0F};
        return glm::vec3{projected} / projected.w;
    };

    EXPECT_NEAR(clip({0.0F, 0.0F, -1.0F}).z, -1.0F, 1e-4F);  // the near plane at -1
    EXPECT_NEAR(clip({0.0F, 0.0F, -10.0F}).z, 1.0F, 1e-4F);  // the far plane at 1
    EXPECT_GT(clip({0.0F, 1.0F, -5.0F}).y, 0.0F);            // up is up: no flip
    const glm::vec4 vulkan = shown.getProjection() * glm::vec4{0.0F, 1.0F, -5.0F, 1.0F};
    EXPECT_LT(vulkan.y, 0.0F);  // where Vulkan's clip space is Y down
}

TEST(Camera3D, ReflectsItsFieldsWithTheProjectionByName) {
    using Traits = aylo::ecs::ComponentTraits<Camera3D>;
    EXPECT_EQ(Traits::kName, "aylo.Camera3D");
    const aylo::ecs::TypeInfo info = aylo::ecs::TypeTraits<Camera3D>::makeTypeInfo();
    ASSERT_EQ(info.fields.size(), 6U);
    EXPECT_EQ(info.fields.at(0).name, "projection");
    ASSERT_EQ(info.fields.at(0).enumNames.size(), 2U);
    EXPECT_EQ(info.fields.at(0).enumNames[1], "orthographic");
    EXPECT_EQ(aylo::ecs::ComponentTraits<Lines3D>::kName, "aylo.Lines3D");
}

TEST(Graphics3D, ShowsTheCamera3DWithTheHighestPriority) {
    BackendApp app;
    aylo::ecs::World& world = app.app().getWorld();
    world.spawn(Transform3D{.position = {1.0F, 0.0F, 0.0F}}, Camera3D{.priority = 1});
    world.spawn(Transform3D{.position = {2.0F, 0.0F, 0.0F}}, Camera3D{.fieldOfView = 90.0F, .priority = 4});

    app.app().runFrame();
    app.app().runFrame();

    ASSERT_FALSE(app.state().views3D.empty());
    const View3D& view = app.state().views3D.back();
    EXPECT_FLOAT_EQ(view.position.x, 2.0F);
    ASSERT_TRUE(std::holds_alternative<Perspective>(view.projection));
    EXPECT_NEAR(std::get<Perspective>(view.projection).fieldOfView, glm::radians(90.0F), 1e-5F);
}

TEST(Graphics3D, AnOrthographicCameraGivesAnOrthographicView) {
    BackendApp app;
    app.app().getWorld().spawn(Transform3D{},
                               Camera3D{.projection = CameraProjection::kOrthographic, .orthographicHeight = 20.0F});

    app.app().runFrame();
    app.app().runFrame();

    ASSERT_FALSE(app.state().views3D.empty());
    ASSERT_TRUE(std::holds_alternative<Orthographic3D>(app.state().views3D.back().projection));
    EXPECT_FLOAT_EQ(std::get<Orthographic3D>(app.state().views3D.back().projection).height, 20.0F);
}

TEST(Graphics3D, ShowsNo3DWorldWithoutACamera3D) {
    BackendApp app;
    app.app().runFrame();
    EXPECT_TRUE(app.state().views3D.empty());
}

TEST(Graphics3D, DrawsTheLinesAtTheirEntity) {
    BackendApp app;
    app.app().getWorld().spawn(Transform3D{.position = {0.0F, 2.0F, 0.0F}, .scale = glm::vec3{3.0F}},
                               Lines3D{.points = {{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}}, .color = {1, 0, 0, 1}});

    app.app().runFrame();
    app.app().runFrame();

    ASSERT_FALSE(app.state().lines3D.empty());
    const aylo::contract::renderer::Lines3D& drawn = app.state().lines3D.back();
    const glm::vec4 end = drawn.world * glm::vec4{drawn.points.at(1), 1.0F};
    EXPECT_FLOAT_EQ(end.x, 3.0F);
    EXPECT_FLOAT_EQ(end.y, 2.0F);
    EXPECT_EQ(drawn.color, (glm::vec4{1, 0, 0, 1}));
}

TEST(Graphics3D, ARendererWithout3DIsToldOnceAndGetsNothing) {
    BackendApp app;
    app.state().draws3D = false;
    app.app().getWorld().spawn(Transform3D{}, Camera3D{});
    app.app().getWorld().spawn(Transform3D{}, Lines3D{.points = {{0, 0, 0}, {1, 0, 0}}, .color = {1, 1, 1, 1}});

    testing::internal::CaptureStderr();
    app.app().runFrame();
    app.app().runFrame();
    app.app().runFrame();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(app.state().views3D.empty());
    EXPECT_TRUE(app.state().lines3D.empty());
    const std::string warning = "does not draw 3D";
    const std::size_t first = log.find(warning);
    ASSERT_NE(first, std::string::npos) << log;
    EXPECT_EQ(log.find(warning, first + 1), std::string::npos) << log;
}

TEST(Light3D, ReflectsItsFieldsWithTheKindByName) {
    using aylo::plugins::graphics::Light3D;
    EXPECT_EQ(aylo::ecs::ComponentTraits<Light3D>::kName, "aylo.Light3D");
    const aylo::ecs::TypeInfo info = aylo::ecs::TypeTraits<Light3D>::makeTypeInfo();
    ASSERT_EQ(info.fields.size(), 10U);
    EXPECT_EQ(info.fields.at(0).name, "kind");
    ASSERT_EQ(info.fields.at(0).enumNames.size(), 4U);
    EXPECT_EQ(info.fields.at(0).enumNames[3], "area");
    EXPECT_EQ(aylo::ecs::ComponentTraits<aylo::plugins::graphics::Environment3D>::kName, "aylo.Environment3D");
}

TEST(Graphics3D, GivesTheRendererTheEnabledLightsInTheWorldAndTheAmbient) {
    using aylo::plugins::graphics::Environment3D;
    using aylo::plugins::graphics::Light3D;
    using aylo::plugins::graphics::LightKind;
    BackendApp app;
    aylo::ecs::World& world = app.app().getWorld();
    // A spot one metre up, turned a quarter to the left: it shines along -X.
    world.spawn(Transform3D{.position = {0.0F, 1.0F, 0.0F},
                            .rotation = {0.0F, std::sin(glm::radians(45.0F)), 0.0F, std::cos(glm::radians(45.0F))}},
                Light3D{.kind = LightKind::kSpot, .power = 500.0F, .innerConeAngle = 10.0F, .outerConeAngle = 30.0F});
    world.spawn(Transform3D{}, Light3D{.kind = LightKind::kDirectional, .intensity = 2.0F});
    world.spawn(Transform3D{}, Light3D{.enabled = false});
    world.spawn(Environment3D{.ambient = {0.1F, 0.1F, 0.2F}, .strength = 0.5F, .priority = 2});
    world.spawn(Environment3D{.ambient = {1.0F, 0.0F, 0.0F}});

    app.app().runFrame();
    app.app().runFrame();

    ASSERT_FALSE(app.state().lightFrames.empty());
    const auto& lights = app.state().lightFrames.back();
    ASSERT_EQ(lights.size(), 2U);
    const auto& spot = lights.at(0);
    EXPECT_EQ(spot.kind, aylo::contract::renderer::LightKind3D::kSpot);
    EXPECT_FLOAT_EQ(spot.position.y, 1.0F);
    EXPECT_NEAR(glm::distance(spot.direction, glm::vec3{-1.0F, 0.0F, 0.0F}), 0.0F, 1e-5F);
    EXPECT_FLOAT_EQ(spot.intensity, 500.0F);
    EXPECT_NEAR(spot.outerCone, glm::radians(30.0F), 1e-5F);
    EXPECT_FLOAT_EQ(lights.at(1).intensity, 2.0F);  // a directional light's lux
    EXPECT_EQ(app.state().environments.back().ambient, glm::vec3(0.1F, 0.1F, 0.2F));
    EXPECT_FLOAT_EQ(app.state().environments.back().strength, 0.5F);
}

TEST(Graphics3D, AGameWithoutLightsGivesTheRendererNone) {
    BackendApp app;
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_TRUE(app.state().lightFrames.empty());
}

TEST(Graphics3D, SaysOnceThatTheRendererDoesNotLight3D) {
    BackendApp app;
    app.state().lights3D = false;
    app.app().getWorld().spawn(Transform3D{}, aylo::plugins::graphics::Light3D{});
    testing::internal::CaptureStderr();
    app.app().runFrame();
    app.app().runFrame();
    const std::string log = testing::internal::GetCapturedStderr();
    EXPECT_EQ(log.find("does not light"), log.rfind("does not light")) << log;
    EXPECT_NE(log.find("does not light"), std::string::npos) << log;
    EXPECT_TRUE(app.state().lightFrames.empty());
}
