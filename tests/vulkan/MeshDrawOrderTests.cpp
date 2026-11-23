/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshDrawOrderTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <vector>

#include "render/vulkan/rendering/MeshDrawOrder.hpp"

using aylo::render::vulkan::rendering::MeshDrawKey;
using aylo::render::vulkan::rendering::MeshViewpoint;
using aylo::render::vulkan::rendering::orderMeshDraws;

TEST(MeshDrawOrder, DrawsTheOpaqueFirstThenTheBlendedBackToFront) {
    const std::array draws{
        MeshDrawKey{.center = {0.0F, 0.0F, -2.0F}, .transparent = true},   // near glass
        MeshDrawKey{.center = {0.0F, 0.0F, -9.0F}, .transparent = false},  // a wall
        MeshDrawKey{.center = {0.0F, 0.0F, -6.0F}, .transparent = true},   // far glass
        MeshDrawKey{.center = {0.0F, 0.0F, -1.0F}, .transparent = false},  // a crate
    };
    std::vector<std::uint32_t> order;
    orderMeshDraws(draws, MeshViewpoint{}, order);
    EXPECT_EQ(order, (std::vector<std::uint32_t>{1, 3, 2, 0}));
}

TEST(MeshDrawOrder, SortsFromAnySide) {
    // Seen from the other side, the two panes of glass swap.
    const std::array draws{MeshDrawKey{.center = {0.0F, 0.0F, -2.0F}, .transparent = true},
                           MeshDrawKey{.center = {0.0F, 0.0F, -6.0F}, .transparent = true}};
    std::vector<std::uint32_t> order;
    orderMeshDraws(draws, MeshViewpoint{.position = {0.0F, 0.0F, -10.0F}, .forward = {0.0F, 0.0F, 1.0F}}, order);
    EXPECT_EQ(order, (std::vector<std::uint32_t>{0, 1}));
    orderMeshDraws(draws, MeshViewpoint{.position = {0.0F, 0.0F, 5.0F}}, order);
    EXPECT_EQ(order, (std::vector<std::uint32_t>{1, 0}));
}

TEST(MeshDrawOrder, AnOrthographicViewSortsAlongItsForward) {
    // Off to the side, the pane farther along the view is drawn first even if it is nearer to the camera's point.
    const std::array draws{MeshDrawKey{.center = {50.0F, 0.0F, -3.0F}, .transparent = true},
                           MeshDrawKey{.center = {0.0F, 0.0F, -5.0F}, .transparent = true}};
    std::vector<std::uint32_t> order;
    orderMeshDraws(draws, MeshViewpoint{.perspective = false}, order);
    EXPECT_EQ(order, (std::vector<std::uint32_t>{1, 0}));
}
