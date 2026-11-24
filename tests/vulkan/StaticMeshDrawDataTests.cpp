/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StaticMeshDrawDataTests
*/

#include <gtest/gtest.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "render/vulkan/rendering/StaticMeshConstants.hpp"
#include "render/vulkan/rendering/StaticMeshDrawData.hpp"

using aylo::contract::renderer::ShaderInstanceValues;
using aylo::contract::renderer::StaticMeshBounds;
using aylo::contract::renderer::StaticMeshDesc;
using aylo::contract::renderer::StaticMeshVertex;
using aylo::render::vulkan::rendering::makeStaticMeshDrawData;
using aylo::render::vulkan::rendering::StaticMeshDrawData;

TEST(StaticMeshBounds, AreTheBoxOfTheVertices) {
    const StaticMeshDesc desc{.vertices = {StaticMeshVertex{.position = {1.0F, -2.0F, 3.0F}},
                                           StaticMeshVertex{.position = {-1.0F, 4.0F, 0.5F}},
                                           StaticMeshVertex{.position = {0.0F, 0.0F, -6.0F}}}};
    const StaticMeshBounds bounds = aylo::contract::renderer::computeBounds(desc);
    EXPECT_EQ(bounds.min, glm::vec3(-1.0F, -2.0F, -6.0F));
    EXPECT_EQ(bounds.max, glm::vec3(1.0F, 4.0F, 3.0F));
    EXPECT_EQ(aylo::contract::renderer::computeBounds(StaticMeshDesc{}), StaticMeshBounds{});
}

TEST(StaticMeshDrawData, CarriesTheModelItsInverseTheColourTheBoundsAndTheInstanceValues) {
    const glm::mat4 world = glm::translate(glm::mat4{1.0F}, glm::vec3{2.0F, 0.0F, -1.0F});
    ShaderInstanceValues instance{};
    instance.at(1) = glm::vec4{0.25F};
    const StaticMeshDrawData data = makeStaticMeshDrawData(
        world, glm::vec4{0.5F}, StaticMeshBounds{.min = glm::vec3{-1.0F}, .max = glm::vec3{1.0F}}, instance);
    EXPECT_EQ(data.model, world);
    const glm::vec4 back = data.inverseModel * world * glm::vec4{3.0F, 4.0F, 5.0F, 1.0F};
    EXPECT_NEAR(glm::distance(back, glm::vec4{3.0F, 4.0F, 5.0F, 1.0F}), 0.0F, 1e-5F);
    EXPECT_EQ(data.color, glm::vec4{0.5F});
    EXPECT_EQ(data.boundsMin, glm::vec4(-1.0F, -1.0F, -1.0F, 1.0F));  // w: shadows fall on it
    EXPECT_EQ(data.boundsMax, glm::vec4(1.0F, 1.0F, 1.0F, 0.0F));
    EXPECT_EQ(data.instance.at(1), glm::vec4{0.25F});
}

TEST(StaticMeshDrawData, KeepsNormalsPerpendicularUnderAnUnevenScale) {
    // A plane x + y = 0 squashed along x: its normal must follow the inverse transpose, not the model matrix.
    const glm::mat4 world = glm::scale(glm::mat4{1.0F}, glm::vec3{2.0F, 1.0F, 1.0F});
    const StaticMeshDrawData data = makeStaticMeshDrawData(world, glm::vec4{1.0F}, StaticMeshBounds{}, {});
    const glm::vec3 tangent = glm::vec3{world * glm::vec4{1.0F, -1.0F, 0.0F, 0.0F}};
    const glm::vec3 normal = glm::vec3{data.normal * glm::vec4{1.0F, 1.0F, 0.0F, 0.0F}};
    EXPECT_NEAR(glm::dot(tangent, normal), 0.0F, 1e-5F);
}

TEST(StaticMeshDrawData, AFlatModelGetsIdentityInversesInsteadOfNaN) {
    const glm::mat4 world = glm::scale(glm::mat4{1.0F}, glm::vec3{0.0F, 1.0F, 1.0F});
    const StaticMeshDrawData data = makeStaticMeshDrawData(world, glm::vec4{1.0F}, StaticMeshBounds{}, {});
    EXPECT_EQ(data.inverseModel, glm::mat4{1.0F});
    EXPECT_EQ(data.normal, glm::mat4{1.0F});
}

TEST(StaticMeshConstants, CarryThePartsMaterialAndWhichMapsItHas) {
    using aylo::contract::renderer::AlphaMode;
    using aylo::contract::renderer::StaticMeshPart;
    using aylo::contract::renderer::TextureId;
    const StaticMeshPart part{.metallic = 0.25F,
                              .roughness = 0.75F,
                              .normalTexture = TextureId{.index = 2, .generation = 1},
                              .normalScale = 0.5F,
                              .emissive = {1.0F, 2.0F, 3.0F},
                              .alphaMode = AlphaMode::kBlend,
                              .alphaCutoff = 0.3F};
    const auto constants = aylo::render::vulkan::rendering::StaticMeshConstants::of(part, 7);
    EXPECT_EQ(constants.draw, 7U);
    EXPECT_EQ(constants.flags, 2U | (2U << 4U));  // a normal map, blended
    EXPECT_FLOAT_EQ(constants.surface.x, 0.25F);
    EXPECT_FLOAT_EQ(constants.surface.w, 0.3F);
    EXPECT_FLOAT_EQ(constants.emissive.w, 0.5F);
}

TEST(StaticMeshDrawData, SaysWhetherShadowsFallOnTheModel) {
    const StaticMeshDrawData data =
        makeStaticMeshDrawData(glm::mat4{1.0F}, glm::vec4{1.0F}, StaticMeshBounds{}, {}, false);
    EXPECT_FLOAT_EQ(data.boundsMin.w, 0.0F);
}
