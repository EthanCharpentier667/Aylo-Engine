/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Transform3DTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformMath.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
using aylo::plugins::transform::GlobalTransform2D;
using aylo::plugins::transform::GlobalTransform3D;
using aylo::plugins::transform::Transform2D;
using aylo::plugins::transform::Transform3D;
using aylo::plugins::transform::TransformInterpolation;

constexpr float kEpsilon = 1e-5F;

/// @return The quaternion (x, y, z, w) of @p angle radians around Y.
glm::vec4 aroundY(const float angle) { return glm::vec4{0.0F, std::sin(angle / 2.0F), 0.0F, std::cos(angle / 2.0F)}; }

void expectNear(const glm::vec3 actual, const glm::vec3 expected) {
    EXPECT_LT(glm::length(actual - expected), kEpsilon) << actual.x << ", " << actual.y << ", " << actual.z;
}

void expectNear(const glm::vec4 actual, const glm::vec4 expected) {
    // A rotation and its negation are the same rotation.
    EXPECT_LT(std::fmin(glm::length(actual - expected), glm::length(actual + expected)), kEpsilon)
        << actual.x << ", " << actual.y << ", " << actual.z << ", " << actual.w;
}

/// @brief An App with the TransformPlugin.
struct App3D {
    aylo::ecs::App app;

    explicit App3D(aylo::ecs::RunMode mode = aylo::ecs::RunMode::kStandalone) : app{mode} {
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
    }

    aylo::ecs::Entity spawn(const Transform3D& transform) {
        const aylo::ecs::Entity entity = app.getWorld().spawn();
        app.getWorld().getComponents().add(entity, transform);
        return entity;
    }

    aylo::ecs::Entity spawnChild(const aylo::ecs::Entity parent, const Transform3D& transform) {
        const aylo::ecs::Entity entity = spawn(transform);
        EXPECT_TRUE(app.getWorld().setParent(entity, parent));
        return entity;
    }

    [[nodiscard]] const GlobalTransform3D* globalOf(const aylo::ecs::Entity entity) {
        return app.getWorld().getComponents().get<GlobalTransform3D>(entity);
    }
};

/// @brief FixedUpdate: moves every 3D root 10 units along X per tick.
void moveAlongX(aylo::ecs::Query<Transform3D> transforms) {
    for (auto [entity, transform] : transforms) {
        transform->position.x += 10.0F;
    }
}
}  // namespace

TEST(Transform3D, ReflectsItsFieldsWithAnIdentityRotation) {
    EXPECT_EQ(aylo::ecs::ComponentTraits<Transform3D>::kName, "aylo.Transform3D");
    EXPECT_EQ(aylo::ecs::ComponentTraits<GlobalTransform3D>::kName, "aylo.GlobalTransform3D");
    EXPECT_EQ(Transform3D{}.rotation, (glm::vec4{0.0F, 0.0F, 0.0F, 1.0F}));
    App3D fixture;
    EXPECT_TRUE(fixture.app.getWorld().getComponents().find("aylo.Transform3D"));
    EXPECT_TRUE(fixture.app.getWorld().getComponents().find("aylo.GlobalTransform3D"));
}

TEST(Transform3DMath, CombinesScaleThenRotationThenPosition) {
    const GlobalTransform3D parent{
        .position = {1.0F, 2.0F, 3.0F}, .rotation = aroundY(glm::half_pi<float>()), .scale = glm::vec3{2.0F}};
    const Transform3D child{.position = {1.0F, 0.0F, 0.0F}, .rotation = aroundY(glm::half_pi<float>()), .scale = {}};

    const GlobalTransform3D world = aylo::plugins::transform::combine(parent, child);

    // A quarter turn around Y takes +X to -Z; doubled by the scale.
    expectNear(world.position, glm::vec3{1.0F, 2.0F, 1.0F});
    expectNear(world.rotation, aroundY(glm::pi<float>()));
}

TEST(Transform3DMath, ToLocalUndoesCombine) {
    const GlobalTransform3D parent{
        .position = {4.0F, -1.0F, 2.0F}, .rotation = aroundY(0.7F), .scale = glm::vec3{1.5F}};
    const Transform3D child{.position = {0.5F, 2.0F, -3.0F}, .rotation = aroundY(-0.3F), .scale = {2.0F, 2.0F, 2.0F}};

    const Transform3D back =
        aylo::plugins::transform::toLocal(parent, aylo::plugins::transform::combine(parent, child));

    expectNear(back.position, child.position);
    expectNear(back.rotation, child.rotation);
    expectNear(back.scale, child.scale);
}

TEST(Transform3DMath, InterpolatesAlongTheShortArc) {
    const Transform3D from{.position = {}, .rotation = aroundY(0.0F), .scale = glm::vec3{1.0F}};
    const Transform3D to{
        .position = {10.0F, 0.0F, 0.0F}, .rotation = aroundY(glm::half_pi<float>()), .scale = glm::vec3{3.0F}};

    const Transform3D half = aylo::plugins::transform::interpolate(from, to, 0.5F);

    expectNear(half.position, glm::vec3{5.0F, 0.0F, 0.0F});
    expectNear(half.rotation, aroundY(glm::quarter_pi<float>()));
    expectNear(half.scale, glm::vec3{2.0F});
}

TEST(Transform3DHierarchy, PlacesAChildInItsParentsAxes) {
    App3D fixture;
    const aylo::ecs::Entity parent = fixture.spawn(Transform3D{
        .position = {0.0F, 5.0F, 0.0F}, .rotation = aroundY(glm::half_pi<float>()), .scale = glm::vec3{1.0F}});
    const aylo::ecs::Entity child = fixture.spawnChild(
        parent, Transform3D{.position = {2.0F, 0.0F, 0.0F}, .rotation = {0, 0, 0, 1}, .scale = {1, 1, 1}});

    fixture.app.runFrame();

    ASSERT_NE(fixture.globalOf(child), nullptr);
    expectNear(fixture.globalOf(child)->position, glm::vec3{0.0F, 5.0F, -2.0F});

    fixture.app.getWorld().getComponents().getMut<Transform3D>(parent)->position.y = 0.0F;
    fixture.app.runFrame();
    expectNear(fixture.globalOf(child)->position, glm::vec3{0.0F, 0.0F, -2.0F});
}

TEST(Transform3DHierarchy, AChildOfA2DEntityIsARoot) {
    App3D fixture;
    aylo::ecs::World& world = fixture.app.getWorld();
    const aylo::ecs::Entity label = world.spawn();
    world.getComponents().add(label,
                              Transform2D{.position = {100.0F, 100.0F}, .rotation = 0.0F, .scale = {1.0F, 1.0F}});
    const aylo::ecs::Entity model = fixture.spawnChild(
        label, Transform3D{.position = {1.0F, 2.0F, 3.0F}, .rotation = {0, 0, 0, 1}, .scale = {1, 1, 1}});

    fixture.app.runFrame();
    fixture.app.runFrame();

    expectNear(fixture.globalOf(model)->position, glm::vec3{1.0F, 2.0F, 3.0F});
    EXPECT_EQ(world.getComponents().get<GlobalTransform2D>(model), nullptr);
    EXPECT_EQ(world.getComponents().get<GlobalTransform3D>(label), nullptr);
}

TEST(Transform3DInterpolation, DrawsBetweenTheLastTwoTicks) {
    App3D fixture;
    fixture.app.addPlugin(
        aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{50}, aylo::ecs::FixedTimestep{10, 5}));
    fixture.app.addSystem(aylo::ecs::Phase::kFixedUpdate, "test.MoveAlongX", &moveAlongX)
        .setSide(aylo::ecs::Side::kBoth);
    const aylo::ecs::Entity drone = fixture.spawn(Transform3D{});
    fixture.app.getWorld().getComponents().add(drone, TransformInterpolation{});

    fixture.app.runFrame();  // no tick: the first pose is recorded
    fixture.app.runFrame();  // tick 1, alpha 0: drawn at the pose of the tick before
    EXPECT_FLOAT_EQ(fixture.globalOf(drone)->position.x, 0.0F);
    fixture.app.runFrame();  // no tick, alpha 0.5: halfway between the last two ticks
    EXPECT_FLOAT_EQ(fixture.app.getWorld().getComponents().get<Transform3D>(drone)->position.x, 10.0F);
    EXPECT_FLOAT_EQ(fixture.globalOf(drone)->position.x, 5.0F);
}
