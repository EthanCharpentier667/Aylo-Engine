/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TransformPluginTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "plugins/transform/Reparent.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
using aylo::plugins::transform::GlobalTransform;
using aylo::plugins::transform::Transform;

/// @brief An App with the TransformPlugin.
struct TransformApp {
    aylo::ecs::App app;  ///< The App under test.

    TransformApp() { app.addPlugin(aylo::plugins::transform::TransformPlugin{}); }

    /// @return The entity's GlobalTransform, or nullptr.
    [[nodiscard]] const GlobalTransform* globalOf(aylo::ecs::Entity entity) {
        return app.getWorld().getComponents().get<GlobalTransform>(entity);
    }

    /// @return A new entity at @p transform.
    aylo::ecs::Entity spawn(const Transform& transform) {
        const aylo::ecs::Entity entity = app.getWorld().spawn();
        app.getWorld().getComponents().add(entity, transform);
        return entity;
    }

    /// @return A new entity at @p transform, child of @p parent.
    aylo::ecs::Entity spawnChild(aylo::ecs::Entity parent, const Transform& transform) {
        const aylo::ecs::Entity entity = spawn(transform);
        EXPECT_TRUE(app.getWorld().setParent(entity, parent));
        return entity;
    }

    /// @brief Moves @p entity to @p position, as gameplay does.
    void move(aylo::ecs::Entity entity, glm::vec2 position) {
        app.getWorld().getComponents().getMut<Transform>(entity)->position = position;
    }

    /// @return Where @p entity is in the world.
    [[nodiscard]] glm::vec2 positionOf(aylo::ecs::Entity entity) {
        const GlobalTransform* global = globalOf(entity);
        EXPECT_NE(global, nullptr);
        return global != nullptr ? global->position : glm::vec2{};
    }
};
}  // namespace

TEST(Transform, ReflectsItsFields) {
    using Traits = aylo::ecs::ComponentTraits<Transform>;
    EXPECT_EQ(Traits::kName, "aylo.Transform");
    ASSERT_EQ(Traits::kFields.size(), 3U);
    EXPECT_EQ(Traits::kFields[0].type, aylo::ecs::FieldType::kVec2);
    EXPECT_EQ(Traits::kFields[1].type, aylo::ecs::FieldType::kF32);
    EXPECT_EQ(Traits::kFields[2].type, aylo::ecs::FieldType::kVec2);
    EXPECT_EQ(aylo::ecs::ComponentTraits<GlobalTransform>::kName, "aylo.GlobalTransform");
}

TEST(TransformPlugin, RegistersTheComponents) {
    TransformApp fixture;
    EXPECT_TRUE(fixture.app.getWorld().getComponents().find("aylo.Transform"));
    EXPECT_TRUE(fixture.app.getWorld().getComponents().find("aylo.GlobalTransform"));
}

TEST(TransformPlugin, GivesAGlobalTransformEqualToTheTransform) {
    TransformApp fixture;
    const aylo::ecs::Entity ship = fixture.app.getWorld().spawn();
    fixture.app.getWorld().getComponents().add(
        ship, Transform{.position = {10.0F, 20.0F}, .rotation = 0.5F, .scale = {2.0F, 3.0F}});

    fixture.app.runFrame();

    const GlobalTransform* global = fixture.globalOf(ship);
    ASSERT_NE(global, nullptr);
    EXPECT_EQ(global->position, (glm::vec2{10.0F, 20.0F}));
    EXPECT_FLOAT_EQ(global->rotation, 0.5F);
    EXPECT_EQ(global->scale, (glm::vec2{2.0F, 3.0F}));
}

TEST(TransformPlugin, FollowsTheTransformWhenItChanges) {
    TransformApp fixture;
    const aylo::ecs::Entity ship = fixture.app.getWorld().spawn();
    fixture.app.getWorld().getComponents().add(ship, Transform{.position = {10.0F, 20.0F}});
    fixture.app.runFrame();

    fixture.app.getWorld().getComponents().getMut<Transform>(ship)->position = {30.0F, 40.0F};
    fixture.app.runFrame();

    ASSERT_NE(fixture.globalOf(ship), nullptr);
    EXPECT_EQ(fixture.globalOf(ship)->position, (glm::vec2{30.0F, 40.0F}));
}

TEST(TransformPlugin, RunsOnEverySide) {
    for (const aylo::ecs::RunMode mode : {aylo::ecs::RunMode::kClient, aylo::ecs::RunMode::kServer}) {
        aylo::ecs::App app{mode};
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        const aylo::ecs::Entity ship = app.getWorld().spawn();
        app.getWorld().getComponents().add(ship, Transform{.position = {1.0F, 2.0F}});
        app.runFrame();
        EXPECT_NE(app.getWorld().getComponents().get<GlobalTransform>(ship), nullptr);
    }
}

TEST(TransformPlugin, LeavesEntitiesWithoutATransformAlone) {
    TransformApp fixture;
    const aylo::ecs::Entity marker = fixture.app.getWorld().spawn();
    fixture.app.runFrame();
    EXPECT_EQ(fixture.globalOf(marker), nullptr);
}

TEST(TransformHierarchy, PlacesAChildOnItsFirstFrame) {
    TransformApp fixture;
    const aylo::ecs::Entity parent = fixture.spawn(Transform{.position = {100.0F, 50.0F}});
    const aylo::ecs::Entity child = fixture.spawnChild(parent, Transform{.position = {10.0F, 5.0F}});
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{110.0F, 55.0F}));
}

TEST(TransformHierarchy, PlacesAChainSpawnedInOneFrameWhateverTheOrder) {
    TransformApp fixture;
    const aylo::ecs::Entity leaf = fixture.spawn(Transform{.position = {1.0F, 0.0F}});
    const aylo::ecs::Entity middle = fixture.spawn(Transform{.position = {10.0F, 0.0F}});
    const aylo::ecs::Entity root = fixture.spawn(Transform{.position = {100.0F, 0.0F}});
    EXPECT_TRUE(fixture.app.getWorld().setParent(leaf, middle));
    EXPECT_TRUE(fixture.app.getWorld().setParent(middle, root));
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(leaf), (glm::vec2{111.0F, 0.0F}));
}

TEST(TransformHierarchy, PlacesANewChildOfAnEntityAlreadyPlaced) {
    TransformApp fixture;
    const aylo::ecs::Entity parent = fixture.spawn(Transform{.position = {100.0F, 0.0F}});
    fixture.app.runFrame();
    const aylo::ecs::Entity child = fixture.spawnChild(parent, Transform{.position = {10.0F, 0.0F}});
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{110.0F, 0.0F}));
}

TEST(TransformHierarchy, MovesTheWholeSubtreeWithItsRoot) {
    TransformApp fixture;
    const aylo::ecs::Entity root = fixture.spawn(Transform{.position = {100.0F, 0.0F}});
    const aylo::ecs::Entity child = fixture.spawnChild(root, Transform{.position = {10.0F, 0.0F}});
    const aylo::ecs::Entity leaf = fixture.spawnChild(child, Transform{.position = {1.0F, 0.0F}});
    fixture.app.runFrame();
    fixture.move(root, {200.0F, 50.0F});
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{210.0F, 50.0F}));
    EXPECT_EQ(fixture.positionOf(leaf), (glm::vec2{211.0F, 50.0F}));
}

TEST(TransformHierarchy, FollowsAChangeOfParent) {
    TransformApp fixture;
    const aylo::ecs::Entity first = fixture.spawn(Transform{.position = {100.0F, 0.0F}});
    const aylo::ecs::Entity second = fixture.spawn(Transform{.position = {500.0F, 0.0F}});
    const aylo::ecs::Entity child = fixture.spawnChild(first, Transform{.position = {10.0F, 0.0F}});
    fixture.app.runFrame();
    EXPECT_TRUE(fixture.app.getWorld().setParent(child, second));
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{510.0F, 0.0F}));
}

TEST(TransformHierarchy, TreatsADetachedChildAsARoot) {
    TransformApp fixture;
    const aylo::ecs::Entity parent = fixture.spawn(Transform{.position = {100.0F, 0.0F}});
    const aylo::ecs::Entity child = fixture.spawnChild(parent, Transform{.position = {10.0F, 0.0F}});
    fixture.app.runFrame();
    EXPECT_TRUE(fixture.app.getWorld().removeParent(child));
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{10.0F, 0.0F}));
}

TEST(TransformHierarchy, TreatsAChildOfAnEntityWithoutTransformAsARoot) {
    TransformApp fixture;
    const aylo::ecs::Entity parent = fixture.app.getWorld().spawn();
    const aylo::ecs::Entity child = fixture.spawnChild(parent, Transform{.position = {10.0F, 0.0F}});
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{10.0F, 0.0F}));
    fixture.move(child, {20.0F, 0.0F});
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{20.0F, 0.0F}));
}

TEST(TransformHierarchy, CombinesRotationAndScale) {
    TransformApp fixture;
    const aylo::ecs::Entity parent = fixture.spawn(Transform{.scale = {2.0F, 2.0F}});
    const aylo::ecs::Entity child =
        fixture.spawnChild(parent, Transform{.position = {10.0F, 0.0F}, .rotation = 0.5F, .scale = {3.0F, 3.0F}});
    fixture.app.runFrame();
    const GlobalTransform* global = fixture.globalOf(child);
    ASSERT_NE(global, nullptr);
    EXPECT_EQ(global->position, (glm::vec2{20.0F, 0.0F}));
    EXPECT_FLOAT_EQ(global->rotation, 0.5F);
    EXPECT_EQ(global->scale, (glm::vec2{6.0F, 6.0F}));
}

TEST(TransformHierarchy, DoesNotMarkAnEntityThatDidNotMoveAsChanged) {
    TransformApp fixture;
    const aylo::ecs::Entity plain = fixture.spawn(Transform{.position = {1.0F, 0.0F}});
    const aylo::ecs::Entity parent = fixture.spawn(Transform{.position = {2.0F, 0.0F}});
    const aylo::ecs::Entity child = fixture.spawnChild(parent, Transform{.position = {3.0F, 0.0F}});
    fixture.app.runFrame();
    fixture.app.runFrame();
    const aylo::ecs::Components& components = fixture.app.getWorld().getComponents();
    const auto plainTicks = components.getTicks<GlobalTransform>(plain);
    const auto childTicks = components.getTicks<GlobalTransform>(child);
    ASSERT_TRUE(plainTicks && childTicks);
    fixture.app.runFrame();
    EXPECT_EQ(components.getTicks<GlobalTransform>(plain)->changed, plainTicks->changed);
    EXPECT_EQ(components.getTicks<GlobalTransform>(child)->changed, childTicks->changed);
}

TEST(TransformHierarchy, DetachInPlaceKeepsAChildWhereItIs) {
    TransformApp fixture;
    const aylo::ecs::Entity parent = fixture.spawn(Transform{.position = {100.0F, 50.0F}, .scale = {2.0F, 2.0F}});
    const aylo::ecs::Entity child = fixture.spawnChild(parent, Transform{.position = {10.0F, 5.0F}});
    fixture.app.runFrame();
    aylo::ecs::Commands commands{fixture.app.getWorld()};
    aylo::plugins::transform::detachInPlace(commands, child, *fixture.globalOf(child));
    commands.flush();
    EXPECT_FALSE(fixture.app.getWorld().getComponents().has<aylo::ecs::Parent>(child));
    fixture.move(parent, {500.0F, 0.0F});
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{120.0F, 60.0F}));
    EXPECT_EQ(fixture.globalOf(child)->scale, (glm::vec2{2.0F, 2.0F}));
}

TEST(TransformHierarchy, AttachInPlaceKeepsAnEntityWhereItIs) {
    TransformApp fixture;
    const aylo::ecs::Entity parent = fixture.spawn(Transform{.position = {100.0F, 50.0F}, .scale = {2.0F, 2.0F}});
    const aylo::ecs::Entity entity = fixture.spawn(Transform{.position = {120.0F, 60.0F}});
    fixture.app.runFrame();
    aylo::ecs::Commands commands{fixture.app.getWorld()};
    aylo::plugins::transform::attachInPlace(commands, entity, *fixture.globalOf(entity), parent,
                                            *fixture.globalOf(parent));
    commands.flush();
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(entity), (glm::vec2{120.0F, 60.0F}));
    fixture.move(parent, {200.0F, 50.0F});
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(entity), (glm::vec2{220.0F, 60.0F}));
}

TEST(TransformPlugin, RegistersTheHierarchy) {
    TransformApp fixture;
    EXPECT_TRUE(fixture.app.getWorld().getComponents().find("aylo.Parent"));
    EXPECT_TRUE(fixture.app.getWorld().getComponents().find("aylo.Children"));
}
