/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorIsolationTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/world/World.hpp"
#include "editor/scene/Isolation.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/graphics/Camera.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
namespace capsule = aylo::plugins::capsule;
namespace scene = aylo::editor::scene;
using aylo::plugins::transform::GlobalTransform2D;
using aylo::plugins::transform::Transform2D;

/// @brief An App with the transform and capsule plugins, and a turret at (50, 20) with a muzzle.
class IsolationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        app.addPlugin(capsule::CapsulePlugin{});
        app.finishPlugins();
        static_cast<void>(app.getWorld().getComponents().registerType<aylo::plugins::graphics::Camera>());
        capsule::Capsule turret{"actors/turret.actor"};
        turret.editNode(0).values.set(app.getWorld().getComponents(), Transform2D{.position = {50.0F, 20.0F}});
        static_cast<void>(turret.addNode("Muzzle", 0));
        app.getWorld().getResources().getMut<capsule::Capsules>()->define(std::move(turret));
    }

    aylo::ecs::World& world() { return app.getWorld(); }

    const capsule::CapsuleId turretId = capsule::CapsuleId::fromPath("actors/turret.actor");
    aylo::ecs::App app;
};
}  // namespace

TEST_F(IsolationTest, SpawnsTheCapsuleUnderAnAnchorFarFromTheGame) {
    std::string error;
    const std::optional<scene::Isolation> isolation = scene::isolate(world(), turretId, error);
    ASSERT_TRUE(isolation.has_value()) << error;
    EXPECT_EQ(isolation->capsule, turretId);
    EXPECT_EQ(world().getComponents().get<Transform2D>(isolation->anchor)->position, scene::kIsolatedOrigin);
    EXPECT_EQ(world().getComponents().get<aylo::ecs::Parent>(isolation->root)->entity, isolation->anchor);
    EXPECT_EQ(world().getComponents().get<Transform2D>(isolation->root)->position, glm::vec2(50.0F, 20.0F));
}

TEST_F(IsolationTest, LooksAtTheCapsulesRoot) {
    std::string error;
    const std::optional<scene::Isolation> isolation = scene::isolate(world(), turretId, error);
    ASSERT_TRUE(isolation.has_value()) << error;
    app.runFrame();  // the global transforms
    EXPECT_EQ(scene::centerOf(world(), *isolation), scene::kIsolatedOrigin + glm::vec2(50.0F, 20.0F));
}

TEST_F(IsolationTest, DespawnsWhatItSpawnedWhenItEnds) {
    std::string error;
    const std::optional<scene::Isolation> isolation = scene::isolate(world(), turretId, error);
    ASSERT_TRUE(isolation.has_value()) << error;
    const std::size_t before = world().getEntityCount();
    EXPECT_EQ(before, 3U);  // the anchor, the turret and its muzzle
    scene::endIsolation(world(), *isolation);
    EXPECT_EQ(world().getEntityCount(), 0U);
}

TEST_F(IsolationTest, TellsWhyACapsuleCannotBeIsolated) {
    std::string error;
    EXPECT_FALSE(scene::isolate(world(), capsule::CapsuleId::fromPath("actors/missing.actor"), error).has_value());
    EXPECT_FALSE(error.empty());
}
