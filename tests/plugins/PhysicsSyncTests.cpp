/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PhysicsSyncTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <string>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryTerms.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/time/Time.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"

using aylo::ecs::Entity;
using aylo::plugins::physics::BodyType;
using aylo::plugins::physics::Collider;
using aylo::plugins::physics::PhysicsPlugin;
using aylo::plugins::physics::PhysicsSettings;
using aylo::plugins::physics::PhysicsWorld;
using aylo::plugins::physics::RigidBody;
using aylo::plugins::physics::ShapeType;
using aylo::plugins::transform::Transform;

namespace {
constexpr float kFixedDelta = 1.0F / 60.0F;
constexpr float kUnitsPerMeter = 100.0F;

/// @brief Counts the Transforms marked as changed after the physics wrote them.
struct ChangedTransforms {
    std::uint32_t count = 0;  ///< Changed Transforms seen since the start.
};
AYLO_ECS_RESOURCE(ChangedTransforms, "test.ChangedTransforms", count);

/// @brief FixedUpdate, after the physics: counts the Transforms changed since it last ran.
void countChanged(aylo::ecs::ResMut<ChangedTransforms> counter,
                  aylo::ecs::Query<const Transform, aylo::ecs::Changed<Transform>> changed) {
    for ([[maybe_unused]] auto [entity, transform] : changed) {
        ++counter->count;
    }
}

/// @brief Settings in world units: 100 units per meter, gravity as given.
PhysicsSettings settingsWith(glm::vec2 gravity) {
    return PhysicsSettings{.gravity = gravity,
                           .unitsPerMeter = kUnitsPerMeter,
                           .maxBodies = 16,
                           .maxBodyPairs = 16,
                           .maxContactConstraints = 16,
                           .tempAllocatorBytes = 1024 * 1024};
}

/// @brief An App with the PhysicsPlugin, ticked by hand.
struct PhysicsApp {
    aylo::ecs::App app;  ///< The App under test.

    explicit PhysicsApp(glm::vec2 gravity = {0.0F, 0.0F}) {
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        app.addPlugin(PhysicsPlugin{settingsWith(gravity)});
        app.getWorld().getResources().insert(aylo::ecs::Time{.fixedDelta = kFixedDelta});
        app.getWorld().getResources().insert(ChangedTransforms{});
        app.addSystem(aylo::ecs::Phase::kFixedUpdate, "test.CountChanged", &countChanged)
            .after(std::string{PhysicsPlugin::kSyncFromPhysicsSystem});
    }

    /// @brief Runs the given number of fixed ticks.
    void tick(int count = 1) {
        for (int index = 0; index < count; ++index) {
            app.getSchedules().run(aylo::ecs::Phase::kFixedUpdate);
        }
    }

    [[nodiscard]] aylo::ecs::Components& components() { return app.getWorld().getComponents(); }
    [[nodiscard]] const PhysicsWorld& world() const { return app.getWorld().getResources().require<PhysicsWorld>(); }
    [[nodiscard]] Transform transformOf(Entity entity) { return *components().get<Transform>(entity); }
    [[nodiscard]] std::uint32_t changedCount() const {
        return app.getWorld().getResources().require<ChangedTransforms>().count;
    }

    /// @return A new entity with a Transform, a circle Collider and, when given, a RigidBody.
    Entity spawn(glm::vec2 position, std::optional<RigidBody> body, float radius = 8.0F) {
        const Entity entity = app.getWorld().spawn();
        components().add(entity, Transform{.position = position});
        components().add(entity, Collider{.shape = ShapeType::kCircle, .radius = radius});
        if (body) {
            components().add(entity, *body);
        }
        return entity;
    }

    /// @brief Moves the entity's Transform, as gameplay does.
    void moveTo(Entity entity, glm::vec2 position) {
        Transform transform = transformOf(entity);
        transform.position = position;
        components().add(entity, transform);
    }
};
}  // namespace

TEST(PhysicsSync, TheTransformOfAFallingBodyFollowsIt) {
    PhysicsApp fixture{{0.0F, 980.0F}};
    const Entity ball = fixture.spawn({0.0F, 0.0F}, RigidBody{.linearDamping = 0.0F});
    fixture.tick(60);

    const Transform transform = fixture.transformOf(ball);
    EXPECT_NEAR(transform.position.x, 0.0F, 1e-3F);
    EXPECT_NEAR(transform.position.y, 490.0F, 20.0F);  // g t^2 / 2 over one second, Y down.
    EXPECT_EQ(fixture.world().getBodyPosition(ball), transform.position);
}

TEST(PhysicsSync, ATransformWrittenByThePhysicsIsNotAMoveByGameplay) {
    PhysicsApp fixture{{0.0F, 980.0F}};
    const Entity ball = fixture.spawn({0.0F, 0.0F}, RigidBody{});
    fixture.tick(10);
    EXPECT_FALSE(fixture.world().isOutOfSync(ball, fixture.transformOf(ball)));
}

TEST(PhysicsSync, MovingTheTransformOfADynamicBodyTeleportsIt) {
    PhysicsApp fixture;
    const Entity ball = fixture.spawn({0.0F, 0.0F}, RigidBody{});
    fixture.tick();

    fixture.moveTo(ball, {500.0F, -200.0F});
    fixture.tick();
    const std::optional<glm::vec2> position = fixture.world().getBodyPosition(ball);
    ASSERT_TRUE(position);
    EXPECT_NEAR(position->x, 500.0F, 1e-2F);
    EXPECT_NEAR(position->y, -200.0F, 1e-2F);
}

TEST(PhysicsSync, MovingTheTransformOfAStaticBodyTeleportsIt) {
    PhysicsApp fixture;
    const Entity wall = fixture.spawn({0.0F, 0.0F}, std::nullopt);
    fixture.tick();

    fixture.moveTo(wall, {50.0F, 60.0F});
    fixture.tick();
    const std::optional<glm::vec2> position = fixture.world().getBodyPosition(wall);
    ASSERT_TRUE(position);
    EXPECT_NEAR(position->x, 50.0F, 1e-3F);
    EXPECT_NEAR(position->y, 60.0F, 1e-3F);
}

TEST(PhysicsSync, AKinematicBodyFollowsItsTransformThenStops) {
    PhysicsApp fixture;
    const Entity platform = fixture.spawn({0.0F, 0.0F}, RigidBody{.type = BodyType::kKinematic});
    fixture.tick();

    for (int step = 1; step <= 10; ++step) {
        fixture.moveTo(platform, {5.0F * static_cast<float>(step), 0.0F});
        fixture.tick();
    }
    std::optional<glm::vec2> position = fixture.world().getBodyPosition(platform);
    ASSERT_TRUE(position);
    EXPECT_NEAR(position->x, 50.0F, 1e-2F);

    fixture.tick(10);  // The Transform no longer moves: neither does the body.
    position = fixture.world().getBodyPosition(platform);
    ASSERT_TRUE(position);
    EXPECT_NEAR(position->x, 50.0F, 1e-2F);
}

TEST(PhysicsSync, AKinematicBodyPushesADynamicOne) {
    PhysicsApp fixture;
    const Entity pusher = fixture.spawn({0.0F, 0.0F}, RigidBody{.type = BodyType::kKinematic}, 16.0F);
    const Entity ball = fixture.spawn({40.0F, 0.0F}, RigidBody{}, 16.0F);
    fixture.tick();

    for (int step = 1; step <= 30; ++step) {
        fixture.moveTo(pusher, {4.0F * static_cast<float>(step), 0.0F});
        fixture.tick();
    }
    EXPECT_GT(fixture.transformOf(ball).position.x, 60.0F);  // Pushed along +X, out of the pusher's way.
}

TEST(PhysicsSync, ABodyAtRestDoesNotMarkItsTransformAsChanged) {
    PhysicsApp fixture;  // No gravity: the ball never moves, and goes to sleep.
    fixture.spawn({0.0F, 0.0F}, RigidBody{});
    fixture.tick(5);
    const std::uint32_t before = fixture.changedCount();
    fixture.tick(60);
    EXPECT_EQ(fixture.changedCount(), before);
}

TEST(PhysicsSync, AFallingBodyMarksItsTransformAsChangedEveryTick) {
    PhysicsApp fixture{{0.0F, 980.0F}};
    fixture.spawn({0.0F, 0.0F}, RigidBody{});
    fixture.tick(2);
    const std::uint32_t before = fixture.changedCount();
    fixture.tick(10);
    EXPECT_EQ(fixture.changedCount(), before + 10);
}
