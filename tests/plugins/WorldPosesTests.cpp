/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** WorldPosesTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/world/World.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "plugins/transform/WorldPoses.hpp"

using aylo::ecs::Entity;
using aylo::ecs::Parent;
using aylo::plugins::transform::GlobalTransform2D;
using aylo::plugins::transform::Transform2D;
using aylo::plugins::transform::WorldPoses;

TEST(WorldPoses, GivesARootItsTransform) {
    aylo::ecs::World world;
    const Entity root = world.spawn();
    const Transform2D local{.position = {3.0F, 4.0F}, .rotation = 0.5F, .scale = {2.0F, 2.0F}};
    WorldPoses poses;
    poses.add(root, local, nullptr);
    const GlobalTransform2D pose = poses.of(root);
    EXPECT_EQ(pose.position, local.position);
    EXPECT_FLOAT_EQ(pose.rotation, local.rotation);
    EXPECT_EQ(pose.scale, local.scale);
}

TEST(WorldPoses, CombinesAWholeChainWhateverTheOrderOfAdd) {
    aylo::ecs::World world;
    const Entity root = world.spawn();
    const Entity middle = world.spawn();
    const Entity leaf = world.spawn();
    const Transform2D rootLocal{.position = {100.0F, 0.0F}};
    const Transform2D middleLocal{.position = {10.0F, 0.0F}};
    const Transform2D leafLocal{.position = {1.0F, 0.0F}};
    const Parent toRoot{.entity = root};
    const Parent toMiddle{.entity = middle};
    WorldPoses poses;
    poses.add(leaf, leafLocal, &toMiddle);
    poses.add(root, rootLocal, nullptr);
    poses.add(middle, middleLocal, &toRoot);
    EXPECT_EQ(poses.of(leaf).position, (glm::vec2{111.0F, 0.0F}));
    EXPECT_EQ(poses.of(middle).position, (glm::vec2{110.0F, 0.0F}));
}

TEST(WorldPoses, CountsTheParentsOfAnEntity) {
    aylo::ecs::World world;
    const Entity root = world.spawn();
    const Entity child = world.spawn();
    const Entity orphan = world.spawn();
    const Transform2D local;
    const Parent toRoot{.entity = root};
    const Parent toUnknown{.entity = world.spawn()};
    WorldPoses poses;
    poses.add(root, local, nullptr);
    poses.add(child, local, &toRoot);
    poses.add(orphan, local, &toUnknown);
    EXPECT_EQ(poses.depthOf(root), 0U);
    EXPECT_EQ(poses.depthOf(child), 1U);
    EXPECT_EQ(poses.depthOf(orphan), 0U);
}

TEST(WorldPoses, GivesTheDefaultToAnEntityNotAdded) {
    aylo::ecs::World world;
    const Entity unknown = world.spawn();
    const WorldPoses poses;
    EXPECT_EQ(poses.of(unknown).position, (glm::vec2{0.0F, 0.0F}));
    EXPECT_EQ(poses.of(unknown).scale, (glm::vec2{1.0F, 1.0F}));
}

TEST(WorldPoses, TreatsAChildOfAParentNotAddedAsARoot) {
    aylo::ecs::World world;
    const Entity unknown = world.spawn();
    const Entity child = world.spawn();
    const Transform2D local{.position = {10.0F, 0.0F}};
    const Parent parent{.entity = unknown};
    WorldPoses poses;
    poses.add(child, local, &parent);
    EXPECT_EQ(poses.of(child).position, (glm::vec2{10.0F, 0.0F}));
}

TEST(WorldPoses, AgreesWithTheTransformPlugin) {
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    app.addPlugin(aylo::plugins::transform::TransformPlugin{});
    aylo::ecs::World& world = app.getWorld();
    const Entity root = world.spawn();
    const Entity middle = world.spawn();
    const Entity leaf = world.spawn();
    const Transform2D rootLocal{.position = {40.0F, -20.0F}, .rotation = 0.7F, .scale = {2.0F, 2.0F}};
    const Transform2D middleLocal{.position = {5.0F, 8.0F}, .rotation = -0.3F, .scale = {-1.0F, 1.0F}};
    const Transform2D leafLocal{.position = {-3.0F, 2.0F}, .rotation = 1.1F, .scale = {0.5F, 0.5F}};
    world.getComponents().add(root, rootLocal);
    world.getComponents().add(middle, middleLocal);
    world.getComponents().add(leaf, leafLocal);
    EXPECT_TRUE(world.setParent(middle, root));
    EXPECT_TRUE(world.setParent(leaf, middle));
    app.runFrame();
    const Parent toRoot{.entity = root};
    const Parent toMiddle{.entity = middle};
    WorldPoses poses;
    poses.add(root, rootLocal, nullptr);
    poses.add(middle, middleLocal, &toRoot);
    poses.add(leaf, leafLocal, &toMiddle);
    const GlobalTransform2D expected = *world.getComponents().get<GlobalTransform2D>(leaf);
    const GlobalTransform2D pose = poses.of(leaf);
    EXPECT_FLOAT_EQ(pose.position.x, expected.position.x);
    EXPECT_FLOAT_EQ(pose.position.y, expected.position.y);
    EXPECT_FLOAT_EQ(pose.rotation, expected.rotation);
    EXPECT_FLOAT_EQ(pose.scale.x, expected.scale.x);
}
