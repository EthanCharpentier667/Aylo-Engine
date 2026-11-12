/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PhysicsPluginTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/time/Time.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/transform/TransformPlugin.hpp"

using aylo::plugins::physics::PhysicsPlugin;
using aylo::plugins::physics::PhysicsSettings;
using aylo::plugins::physics::PhysicsWorld;

namespace {
constexpr float kFixedDelta = 1.0F / 60.0F;

/// @brief Small capacities, so tests build worlds quickly.
PhysicsSettings smallSettings() {
    return PhysicsSettings{
        .maxBodies = 16, .maxBodyPairs = 16, .maxContactConstraints = 16, .tempAllocatorBytes = 1024 * 1024};
}
}  // namespace

TEST(PhysicsPlugin, IsNamedAyloPhysics) { EXPECT_EQ(PhysicsPlugin{}.getName(), "aylo.Physics"); }

TEST(PhysicsPlugin, RegistersTheComponents) {
    aylo::ecs::App app;
    app.addPlugin(aylo::plugins::transform::TransformPlugin{});
    app.addPlugin(PhysicsPlugin{smallSettings()});
    EXPECT_TRUE(app.getWorld().getComponents().find("aylo.RigidBody"));
    EXPECT_TRUE(app.getWorld().getComponents().find("aylo.Collider"));
    EXPECT_TRUE(app.getWorld().getComponents().find("aylo.Velocity"));
}

TEST(PhysicsPlugin, InsertsTheWorldFromItsSettings) {
    aylo::ecs::App app;
    PhysicsSettings settings = smallSettings();
    settings.unitsPerMeter = 50.0F;
    app.addPlugin(aylo::plugins::transform::TransformPlugin{});
    app.addPlugin(PhysicsPlugin{settings});

    const PhysicsWorld* const world = app.getWorld().getResources().get<PhysicsWorld>();
    ASSERT_NE(world, nullptr);
    EXPECT_FLOAT_EQ(world->getUnitsPerMeter(), 50.0F);
}

TEST(PhysicsPlugin, AddsTheStepSystemInFixedUpdate) {
    aylo::ecs::App app;
    app.addPlugin(aylo::plugins::transform::TransformPlugin{});
    app.addPlugin(PhysicsPlugin{smallSettings()});
    EXPECT_TRUE(app.getSchedules().getSchedule(aylo::ecs::Phase::kFixedUpdate).contains(PhysicsPlugin::kStepSystem));
}

TEST(PhysicsPlugin, StepsTheWorldOncePerFixedUpdate) {
    aylo::ecs::App app;
    app.addPlugin(aylo::plugins::transform::TransformPlugin{});
    app.addPlugin(PhysicsPlugin{smallSettings()});
    app.getWorld().getResources().insert(aylo::ecs::Time{.fixedDelta = kFixedDelta});

    app.getSchedules().run(aylo::ecs::Phase::kFixedUpdate);
    app.getSchedules().run(aylo::ecs::Phase::kFixedUpdate);

    EXPECT_EQ(app.getWorld().getResources().require<PhysicsWorld>().getStepCount(), 2U);
}

TEST(PhysicsPlugin, RejectsInvalidSettingsWhenBuilt) {
    aylo::ecs::App app;
    PhysicsSettings settings = smallSettings();
    settings.unitsPerMeter = 0.0F;
    app.addPlugin(aylo::plugins::transform::TransformPlugin{});
    EXPECT_THROW(app.addPlugin(PhysicsPlugin{settings}), aylo::plugins::physics::exceptions::PhysicsException);
}
