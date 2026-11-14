/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Physics3DTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/system/EventReader.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/time/Time.hpp"
#include "aylo/ecs/world/World.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/Collider3D.hpp"
#include "plugins/physics/CollisionEvents.hpp"
#include "plugins/physics/PhysicsHandle.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity3D.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"

using aylo::plugins::physics::BodyType;
using aylo::plugins::physics::Collider;
using aylo::plugins::physics::Collider3D;
using aylo::plugins::physics::PhysicsPlugin;
using aylo::plugins::physics::PhysicsSettings;
using aylo::plugins::physics::PhysicsWorld;
using aylo::plugins::physics::RigidBody;
using aylo::plugins::physics::Shape3DType;
using aylo::plugins::physics::Velocity3D;
using aylo::plugins::transform::Transform2D;
using aylo::plugins::transform::Transform3D;

namespace {
constexpr float kFixedDelta = 1.0F / 60.0F;

/// @brief Small capacities, so tests build worlds quickly; @p gravity is the 2D gravity.
PhysicsSettings smallSettings(glm::vec2 gravity = {0.0F, 0.0F}) {
    return PhysicsSettings{.gravity = gravity,
                           .maxBodies = 16,
                           .maxBodyPairs = 16,
                           .maxContactConstraints = 16,
                           .tempAllocatorBytes = 1024 * 1024};
}

/// @brief The CollisionStarted events the plugin sent.
struct Received {
    std::vector<aylo::plugins::physics::CollisionStarted> started;  ///< In order.
};
AYLO_ECS_RESOURCE(Received, "test.Received3D");

/// @brief FixedUpdate, after the physics: records the collisions.
void record(aylo::ecs::ResMut<Received> received,
            aylo::ecs::EventReader<aylo::plugins::physics::CollisionStarted> started) {
    for (const aylo::plugins::physics::CollisionStarted& event : started) {
        received->started.push_back(event);
    }
}

/// @brief An App with the physics, ticked by hand one fixed step at a time, recording the collisions.
class Physics3DApp {
  public:
    explicit Physics3DApp(PhysicsSettings settings = smallSettings(),
                          aylo::ecs::RunMode mode = aylo::ecs::RunMode::kStandalone)
        : _app{mode} {
        _app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        _app.addPlugin(PhysicsPlugin{settings});
        _app.getWorld().getResources().insert(aylo::ecs::Time{.fixedDelta = kFixedDelta});
        _app.insertResource(Received{});
        _app.addSystem(aylo::ecs::Phase::kFixedUpdate, "test.Record", &record)
            .after(std::string{PhysicsPlugin::kEventsSystem});
    }

    [[nodiscard]] aylo::ecs::World& world() noexcept { return _app.getWorld(); }
    [[nodiscard]] aylo::ecs::Components& components() { return _app.getWorld().getComponents(); }
    [[nodiscard]] PhysicsWorld& physics() { return *_app.getWorld().getResources().getMut<PhysicsWorld>(); }

    /// @return A new entity with @p components.
    template <typename... Components>
    aylo::ecs::Entity spawn(Components... components) {
        const aylo::ecs::Entity entity = _app.getWorld().spawn();
        (this->components().add(entity, components), ...);
        return entity;
    }

    void tick(int count = 1) {
        for (int index = 0; index < count; ++index) {
            _app.getSchedules().run(aylo::ecs::Phase::kFixedUpdate);
        }
    }

    template <typename Component>
    [[nodiscard]] Component get(aylo::ecs::Entity entity) {
        return *components().get<Component>(entity);
    }
    [[nodiscard]] Transform3D transformOf(aylo::ecs::Entity entity) { return get<Transform3D>(entity); }

    /// @return The collisions sent since the last call.
    [[nodiscard]] std::vector<aylo::plugins::physics::CollisionStarted> collisions() {
        Received& received = *_app.getWorld().getResources().getMut<Received>();
        std::vector<aylo::plugins::physics::CollisionStarted> started = std::move(received.started);
        received.started.clear();
        return started;
    }

  private:
    aylo::ecs::App _app;
};

/// @brief A static floor: a box 10 metres wide, its top at y = 0.
aylo::ecs::Entity spawnFloor(Physics3DApp& fixture) {
    return fixture.spawn(Transform3D{.position = {0.0F, -0.5F, 0.0F}},
                         Collider3D{.shape = Shape3DType::kBox, .size = {10.0F, 1.0F, 10.0F}});
}
}  // namespace

TEST(Physics3D, RegistersTheComponents) {
    Physics3DApp fixture;
    EXPECT_TRUE(fixture.world().getComponents().find("aylo.Collider3D"));
    EXPECT_TRUE(fixture.world().getComponents().find("aylo.Velocity3D"));
}

TEST(Physics3D, ASphereFallsOnABoxAndRests) {
    Physics3DApp fixture;
    spawnFloor(fixture);
    const aylo::ecs::Entity ball =
        fixture.spawn(Transform3D{.position = {0.0F, 3.0F, 0.0F}},
                      Collider3D{.shape = Shape3DType::kSphere, .radius = 0.5F}, RigidBody{});

    fixture.tick(180);

    const Transform3D resting = fixture.transformOf(ball);
    EXPECT_NEAR(resting.position.y, 0.5F, 0.05F);  // its radius above the floor's top
    EXPECT_NEAR(resting.position.x, 0.0F, 1e-3F);
    const std::optional<Velocity3D> velocity = fixture.physics().getBodyVelocity3D(ball);
    ASSERT_TRUE(velocity);
    EXPECT_NEAR(velocity->linear.y, 0.0F, 0.05F);
}

TEST(Physics3D, FallsWithTheGravity3D) {
    Physics3DApp fixture;
    const aylo::ecs::Entity ball = fixture.spawn(Transform3D{}, Collider3D{.shape = Shape3DType::kSphere},
                                                 RigidBody{.linearDamping = 0.0F}, Velocity3D{});

    fixture.tick(60);

    // About 9.81 m/s after a second, by the default gravity3D, and no 2D gravity is set.
    const Velocity3D velocity = fixture.get<Velocity3D>(ball);
    EXPECT_NEAR(velocity.linear.y, -9.81F, 0.2F);
    EXPECT_LT(fixture.transformOf(ball).position.y, -4.0F);
}

TEST(Physics3D, TheGravityScaleAndSetGravity3DChangeTheFall) {
    Physics3DApp fixture;
    const aylo::ecs::Entity floating =
        fixture.spawn(Transform3D{}, Collider3D{.shape = Shape3DType::kSphere}, RigidBody{.gravityScale = 0.0F});
    const aylo::ecs::Entity sideways = fixture.spawn(Transform3D{.position = {5.0F, 0.0F, 0.0F}},
                                                     Collider3D{.shape = Shape3DType::kSphere}, RigidBody{});
    fixture.physics().setGravity3D({4.0F, 0.0F, 0.0F});

    fixture.tick(30);

    EXPECT_NEAR(fixture.transformOf(floating).position.x, 0.0F, 1e-4F);
    EXPECT_GT(fixture.transformOf(sideways).position.x, 5.3F);
    EXPECT_NEAR(fixture.transformOf(sideways).position.y, 0.0F, 1e-4F);
}

TEST(Physics3D, A2DBodyAndA3DBodyAtTheSamePlaceDoNotCollide) {
    Physics3DApp fixture{smallSettings({0.0F, 0.0F})};
    fixture.physics().setGravity3D({0.0F, 0.0F, 0.0F});
    const aylo::ecs::Entity flat = fixture.spawn(
        Transform2D{}, Collider{.shape = aylo::plugins::physics::ShapeType::kCircle, .radius = 32.0F}, RigidBody{});
    const aylo::ecs::Entity solid =
        fixture.spawn(Transform3D{}, Collider3D{.shape = Shape3DType::kSphere, .radius = 1.0F}, RigidBody{});

    fixture.tick(10);

    EXPECT_TRUE(fixture.collisions().empty());
    EXPECT_EQ(fixture.transformOf(solid).position, glm::vec3{0.0F});
    EXPECT_EQ(fixture.get<Transform2D>(flat).position, glm::vec2{0.0F});
}

TEST(Physics3D, Two3DBodiesReportWhereTheyTouch) {
    Physics3DApp fixture;
    spawnFloor(fixture);
    fixture.spawn(Transform3D{.position = {0.0F, 0.6F, 0.0F}},
                  Collider3D{.shape = Shape3DType::kSphere, .radius = 0.5F}, RigidBody{});

    std::vector<aylo::plugins::physics::CollisionStarted> started;
    for (int tick = 0; tick < 30 && started.empty(); ++tick) {
        fixture.tick();
        started = fixture.collisions();
    }

    ASSERT_EQ(started.size(), 1U);
    EXPECT_NEAR(started.front().point3D.y, 0.0F, 0.05F);
    EXPECT_NEAR(std::abs(started.front().normal3D.y), 1.0F, 1e-3F);
    EXPECT_EQ(started.front().point, glm::vec2{0.0F});  // the 2D fields stay zero
}

TEST(Physics3D, ARay3DSeesOnlyThe3DBodies) {
    Physics3DApp fixture;
    const aylo::ecs::Entity floor = spawnFloor(fixture);
    fixture.spawn(Transform2D{.position = {0.0F, -64.0F}}, Collider{.size = {64.0F, 64.0F}});
    fixture.tick();

    const std::optional<PhysicsWorld::RayHit3D> hit =
        fixture.physics().raycast3D({0.0F, 5.0F, 0.0F}, {0.0F, -2.0F, 0.0F}, 10.0F);

    ASSERT_TRUE(hit);
    EXPECT_EQ(hit->entity, floor);
    EXPECT_NEAR(hit->distance, 5.0F, 1e-3F);
    EXPECT_NEAR(hit->normal.y, 1.0F, 1e-3F);
    EXPECT_EQ(fixture.physics().raycastAll3D({0.0F, 5.0F, 0.0F}, {0.0F, -1.0F, 0.0F}, 10.0F).size(), 1U);
    // A 2D ray along the same line sees the 2D box only.
    const std::optional<PhysicsWorld::RayHit> flat = fixture.physics().raycast({0.0F, -200.0F}, {0.0F, 1.0F}, 400.0F);
    ASSERT_TRUE(flat);
    EXPECT_NE(flat->entity, floor);
}

TEST(Physics3D, AKinematicBodyMovesAtItsVelocity3D) {
    Physics3DApp fixture;
    const aylo::ecs::Entity platform =
        fixture.spawn(Transform3D{}, Collider3D{}, RigidBody{.type = BodyType::kKinematic},
                      Velocity3D{.linear = {0.0F, 0.0F, -2.0F}});

    fixture.tick(31);

    EXPECT_NEAR(fixture.transformOf(platform).position.z, -1.0F, 0.05F);
}

TEST(Physics3D, MovingTheTransform3DTeleportsTheBody) {
    Physics3DApp fixture;
    const aylo::ecs::Entity box = fixture.spawn(Transform3D{}, Collider3D{}, RigidBody{.gravityScale = 0.0F});
    fixture.tick();

    fixture.components().add(box, Transform3D{.position = {3.0F, 2.0F, 1.0F}});
    fixture.tick();

    const std::optional<PhysicsWorld::BodyPose3D> pose = fixture.physics().getBodyPose3D(box);
    ASSERT_TRUE(pose);
    EXPECT_NEAR(pose->position.x, 3.0F, 1e-4F);
    EXPECT_NEAR(pose->position.z, 1.0F, 1e-4F);
}

TEST(Physics3D, AnImpulseAndATorqueMoveAndTurnTheBody) {
    Physics3DApp fixture;
    fixture.physics().setGravity3D({0.0F, 0.0F, 0.0F});
    const aylo::ecs::Entity box =
        fixture.spawn(Transform3D{}, Collider3D{}, RigidBody{.mass = 2.0F, .linearDamping = 0.0F}, Velocity3D{});
    fixture.tick();

    fixture.physics().applyImpulse3D(box, {4.0F, 0.0F, 0.0F});
    fixture.physics().applyTorque3D(box, {0.0F, 50.0F, 0.0F});
    fixture.tick();

    const Velocity3D velocity = fixture.get<Velocity3D>(box);
    EXPECT_NEAR(velocity.linear.x, 2.0F, 1e-3F);  // 4 kg m/s on 2 kg
    EXPECT_GT(velocity.angular.y, 0.0F);
}

TEST(Physics3D, AnEntityWithBothCollidersGetsOne3DBody) {
    Physics3DApp fixture;
    const aylo::ecs::Entity both = fixture.spawn(Transform2D{}, Collider{}, Transform3D{}, Collider3D{});
    fixture.tick();
    const auto* handle = fixture.components().get<aylo::plugins::physics::PhysicsHandle>(both);
    ASSERT_NE(handle, nullptr);
    const auto first = handle->body;

    fixture.tick(3);

    EXPECT_EQ(fixture.physics().getBodyCount(), 1U);
    EXPECT_TRUE(fixture.physics().getBodyPose3D(both));
    EXPECT_FALSE(fixture.physics().getBodyPose(both));
    EXPECT_EQ(fixture.get<aylo::plugins::physics::PhysicsHandle>(both).body, first);  // not rebuilt
}

TEST(Physics3D, AnInvalidShapeIsRefused) {
    Physics3DApp fixture;
    const aylo::ecs::Entity entity = fixture.spawn(Transform3D{});
    EXPECT_THROW(static_cast<void>(fixture.physics().createBody3D(
                     entity, Collider3D{.shape = Shape3DType::kSphere, .radius = 0.0F}, nullptr, Transform3D{})),
                 aylo::plugins::physics::exceptions::PhysicsException);
    EXPECT_THROW(static_cast<void>(fixture.physics().createBody3D(entity, Collider3D{.size = {1.0F, -1.0F, 1.0F}},
                                                                  nullptr, Transform3D{})),
                 aylo::plugins::physics::exceptions::PhysicsException);
}

TEST(Physics3D, OnAClientTheBodiesFollowTheirTransform3DWithoutFalling) {
    Physics3DApp fixture{smallSettings(), aylo::ecs::RunMode::kClient};
    const aylo::ecs::Entity ball = fixture.spawn(Transform3D{.position = {0.0F, 2.0F, 0.0F}},
                                                 Collider3D{.shape = Shape3DType::kSphere}, RigidBody{});
    fixture.tick(10);
    EXPECT_FLOAT_EQ(fixture.transformOf(ball).position.y, 2.0F);

    fixture.components().add(ball, Transform3D{.position = {4.0F, 2.0F, 0.0F}});  // as replication writes it
    fixture.tick();

    const std::optional<PhysicsWorld::RayHit3D> hit =
        fixture.physics().raycast3D({4.0F, 10.0F, 0.0F}, {0.0F, -1.0F, 0.0F}, 20.0F);
    ASSERT_TRUE(hit);
    EXPECT_EQ(hit->entity, ball);
}
