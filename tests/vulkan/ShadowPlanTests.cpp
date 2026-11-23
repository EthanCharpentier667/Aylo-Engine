/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShadowPlanTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>
#include <vector>

#include "aylo/contract/renderer/drawing/Light3D.hpp"
#include "render/vulkan/rendering/ShadowPlan.hpp"

using aylo::contract::renderer::Environment3D;
using aylo::contract::renderer::Light3D;
using aylo::contract::renderer::LightKind3D;
using aylo::render::vulkan::rendering::planShadows;
using aylo::render::vulkan::rendering::ShadowCamera;
using aylo::render::vulkan::rendering::ShadowPlan;

namespace {
/// @return A camera at @p position looking along -Z, 60° high.
ShadowCamera cameraAt(const glm::vec3& position) {
    return ShadowCamera{.view = glm::lookAt(position, position + glm::vec3{0, 0, -1}, glm::vec3{0, 1, 0}),
                        .verticalFov = 1.0472F,
                        .aspect = 16.0F / 9.0F};
}

/// @brief A sun turned down.
const Light3D kSun{
    .kind = LightKind3D::kDirectional, .direction = glm::normalize(glm::vec3{0.3F, -1.0F, -0.2F}), .castShadows = true};
}  // namespace

TEST(ShadowPlan, GivesASunOneMapPerCascadeOverTheShadowDistance) {
    const ShadowPlan plan = planShadows(std::vector{kSun}, Environment3D{.shadowCascades = 3, .shadowDistance = 40.0F},
                                        cameraAt(glm::vec3{0.0F}));
    ASSERT_EQ(plan.tiles.size(), 3U);
    EXPECT_EQ(plan.cascades, 3U);
    EXPECT_EQ(plan.firstTile.at(0), 0);
    EXPECT_LT(plan.tiles.at(0).depth.w, plan.tiles.at(1).depth.w);
    EXPECT_NEAR(plan.tiles.at(2).depth.w, 40.0F, 1e-3F);  // the last cascade ends at the distance
    EXPECT_FLOAT_EQ(plan.tiles.at(1).rect.x, 0.25F);      // 1024 texels: a 4 by 4 grid
}

TEST(ShadowPlan, APointLightHasSixMapsAndASpotOneLookingAlongIt) {
    const Light3D spot{.kind = LightKind3D::kSpot, .position = {0, 5, 0}, .direction = {0, -1, 0}, .castShadows = true};
    const Light3D point{.kind = LightKind3D::kPoint, .castShadows = true};
    const Light3D unshadowed{};
    const ShadowPlan plan = planShadows(std::vector{unshadowed, spot, point}, Environment3D{}, cameraAt(glm::vec3{0}));
    EXPECT_EQ(plan.firstTile, (std::vector<std::int32_t>{-1, 0, 1}));
    ASSERT_EQ(plan.tiles.size(), 7U);
    // The point straight below the spot is at the centre of its map.
    const glm::vec4 clip = plan.tiles.at(0).viewProjection * glm::vec4{0.0F, 0.0F, 0.0F, 1.0F};
    EXPECT_NEAR(clip.x / clip.w, 0.0F, 1e-4F);
    EXPECT_NEAR(clip.y / clip.w, 0.0F, 1e-4F);
    EXPECT_GT(clip.z / clip.w, 0.0F);
    EXPECT_LT(clip.z / clip.w, 1.0F);
}

TEST(ShadowPlan, ALightThatDoesNotFitCastsNone) {
    // 2048 texels: four maps; a point light needs six.
    const Light3D point{.kind = LightKind3D::kPoint, .castShadows = true};
    const ShadowPlan plan =
        planShadows(std::vector{point}, Environment3D{.shadowResolution = 2048}, cameraAt(glm::vec3{0}));
    EXPECT_TRUE(plan.tiles.empty());
    EXPECT_EQ(plan.firstTile.at(0), -1);
}

TEST(ShadowPlan, ACascadeDoesNotMoveWhenTheCameraMovesLessThanATexel) {
    const Environment3D environment{.shadowCascades = 1, .shadowDistance = 20.0F};
    const ShadowPlan before = planShadows(std::vector{kSun}, environment, cameraAt(glm::vec3{0.0F}));
    const ShadowPlan after = planShadows(std::vector{kSun}, environment, cameraAt(glm::vec3{0.0F, 0.0F, 0.0F}));
    EXPECT_EQ(before.tiles.at(0).viewProjection, after.tiles.at(0).viewProjection);
    const float texel = before.tiles.at(0).size.x / 1024.0F;
    const ShadowPlan moved =
        planShadows(std::vector{kSun}, environment, cameraAt(glm::vec3{texel * 0.01F, 0.0F, 0.0F}));
    // The same texels, so the edges do not shimmer: the projection moved by a whole number of texels (here none).
    const glm::vec4 a = before.tiles.at(0).viewProjection * glm::vec4{1.0F, 0.0F, -5.0F, 1.0F};
    const glm::vec4 b = moved.tiles.at(0).viewProjection * glm::vec4{1.0F, 0.0F, -5.0F, 1.0F};
    const float shift = (b.x - a.x) * 0.5F * 1024.0F;  // in texels
    EXPECT_NEAR(shift, std::round(shift), 1e-2F);
}
