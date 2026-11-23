/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorLightGizmosTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float3.hpp>
#include <vector>

#include "aylo/contract/renderer/drawing/Lines3D.hpp"
#include "aylo/ecs/world/World.hpp"
#include "editor/scene/LightGizmos.hpp"
#include "plugins/graphics/Light.hpp"
#include "plugins/transform/Transform.hpp"

using aylo::plugins::graphics::Light3D;
using aylo::plugins::graphics::LightKind;
using aylo::plugins::transform::GlobalTransform3D;

TEST(EditorLightGizmos, DrawsEachLightInItsColourAndDisabledOnesInGrey) {
    aylo::ecs::World world;
    static_cast<void>(world.getComponents().registerType<Light3D>());
    static_cast<void>(world.getComponents().registerType<GlobalTransform3D>());
    world.spawn(GlobalTransform3D{.position = {0.0F, 3.0F, 0.0F}},
                Light3D{.kind = LightKind::kSpot, .color = {1.0F, 0.5F, 0.0F}, .range = 4.0F});
    world.spawn(GlobalTransform3D{}, Light3D{.kind = LightKind::kArea, .enabled = false});
    world.spawn(Light3D{});  // no world pose: not drawn

    const std::vector<aylo::contract::renderer::Lines3D> gizmos = aylo::editor::scene::lightGizmos(world);
    ASSERT_EQ(gizmos.size(), 2U);
    EXPECT_FLOAT_EQ(gizmos.at(0).color.y, 0.5F);
    EXPECT_FLOAT_EQ(gizmos.at(0).world[3][1], 3.0F);
    EXPECT_EQ(gizmos.at(0).points.size() % 2, 0U);
    EXPECT_GT(gizmos.at(0).points.size(), 8U);
    EXPECT_FLOAT_EQ(gizmos.at(1).color.x, 0.5F);  // grey
    EXPECT_EQ(gizmos.at(1).points.size(), 10U);   // the rectangle and the way it faces
}
