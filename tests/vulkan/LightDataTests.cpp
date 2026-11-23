/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** LightDataTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <cstring>
#include <glm/ext/vector_float3.hpp>
#include <vector>

#include "aylo/contract/renderer/drawing/Light3D.hpp"
#include "render/vulkan/rendering/LightData.hpp"

using aylo::contract::renderer::Environment3D;
using aylo::contract::renderer::kMaxLights3D;
using aylo::contract::renderer::Light3D;
using aylo::contract::renderer::LightKind3D;
using aylo::render::vulkan::rendering::GpuLight;
using aylo::render::vulkan::rendering::GpuLightsHeader;
using aylo::render::vulkan::rendering::packLights;

namespace {
/// @return The header of @p bytes.
GpuLightsHeader headerOf(const std::vector<std::byte>& bytes) {
    GpuLightsHeader header;
    std::memcpy(&header, bytes.data(), sizeof(header));
    return header;
}

/// @return Light @p index of @p bytes.
GpuLight lightOf(const std::vector<std::byte>& bytes, std::size_t index) {
    GpuLight light;
    std::memcpy(&light, bytes.data() + sizeof(GpuLightsHeader) + (index * sizeof(GpuLight)), sizeof(light));
    return light;
}
}  // namespace

TEST(LightData, PacksTheAmbientAndEachLightAsTheShadersReadThem) {
    const std::vector<Light3D> lights{Light3D{.kind = LightKind3D::kSpot,
                                              .position = {1.0F, 2.0F, 3.0F},
                                              .color = {1.0F, 0.5F, 0.25F},
                                              .intensity = 300.0F,
                                              .range = 10.0F,
                                              .innerCone = 0.2F,
                                              .outerCone = 0.5F,
                                              .size = {2.0F, 4.0F},
                                              .castShadows = true}};
    std::vector<std::byte> bytes;
    packLights(lights, Environment3D{.ambient = {0.2F, 0.2F, 0.4F}, .strength = 0.5F}, glm::vec3{0.0F}, bytes);
    ASSERT_EQ(bytes.size(), sizeof(GpuLightsHeader) + sizeof(GpuLight));
    const GpuLightsHeader header = headerOf(bytes);
    EXPECT_EQ(header.count.x, 1U);
    EXPECT_FLOAT_EQ(header.ambient.z, 0.2F);
    const GpuLight light = lightOf(bytes, 0);
    EXPECT_FLOAT_EQ(light.position.w, 10.0F);
    EXPECT_FLOAT_EQ(light.direction.w, 2.0F);  // kSpot
    EXPECT_FLOAT_EQ(light.color.w, 300.0F);
    EXPECT_FLOAT_EQ(light.cone.x, std::cos(0.2F));
    EXPECT_FLOAT_EQ(light.cone.y, std::cos(0.5F));
    EXPECT_FLOAT_EQ(light.cone.w, 2.0F);  // half the height
    EXPECT_FLOAT_EQ(light.right.w, 1.0F);
}

TEST(LightData, LeavesOutALightThatLightsNothing) {
    const std::vector<Light3D> lights{Light3D{.intensity = 0.0F}, Light3D{.color = glm::vec3{0.0F}}, Light3D{}};
    std::vector<std::byte> bytes;
    packLights(lights, Environment3D{}, glm::vec3{0.0F}, bytes);
    EXPECT_EQ(headerOf(bytes).count.x, 1U);
}

TEST(LightData, PastTheCapKeepsTheSunsThenTheNearestLights) {
    std::vector<Light3D> lights;
    for (std::size_t index = 0; index < kMaxLights3D + 10; ++index) {
        lights.push_back(Light3D{.position = {static_cast<float>(index) + 1.0F, 0.0F, 0.0F}});
    }
    lights.push_back(Light3D{.kind = LightKind3D::kDirectional});
    std::vector<std::byte> bytes;
    packLights(lights, Environment3D{}, glm::vec3{0.0F}, bytes);
    ASSERT_EQ(headerOf(bytes).count.x, kMaxLights3D);
    EXPECT_FLOAT_EQ(lightOf(bytes, 0).direction.w, 0.0F);  // the sun first
    float farthest = 0.0F;
    for (std::size_t index = 1; index < kMaxLights3D; ++index) {
        farthest = std::max(farthest, lightOf(bytes, index).position.x);
    }
    EXPECT_FLOAT_EQ(farthest, static_cast<float>(kMaxLights3D - 1));  // the 10 farthest left out
}
