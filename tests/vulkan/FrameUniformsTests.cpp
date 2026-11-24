/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FrameUniformsTests
*/

#include <gtest/gtest.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>

#include "render/vulkan/rendering/FrameUniforms.hpp"

using aylo::render::vulkan::rendering::FrameUniforms;

TEST(FrameUniforms, SetCameraGivesTheMatricesTheirInversesAndWhereTheCameraIs) {
    const glm::vec3 eye{3.0F, 2.0F, 5.0F};
    const glm::mat4 view = glm::lookAt(eye, glm::vec3{0.0F}, glm::vec3{0.0F, 1.0F, 0.0F});
    const glm::mat4 projection = glm::perspective(1.0F, 1.5F, 0.1F, 100.0F);
    FrameUniforms uniforms;
    aylo::render::vulkan::rendering::setCamera(uniforms, view, projection, true);

    EXPECT_EQ(uniforms.viewProjection, projection * view);
    EXPECT_NEAR(glm::distance(glm::vec3{uniforms.cameraPosition}, eye), 0.0F, 1e-4F);
    EXPECT_FLOAT_EQ(uniforms.cameraPosition.w, 1.0F);
    const glm::vec4 point{1.0F, -2.0F, 0.5F, 1.0F};
    EXPECT_NEAR(glm::distance(uniforms.inverseView * (view * point), point), 0.0F, 1e-4F);
    EXPECT_NEAR(glm::distance(uniforms.inverseProjection * (projection * point), point), 0.0F, 1e-3F);
}

TEST(FrameUniforms, AnOrthographicCameraSaysSo) {
    FrameUniforms uniforms;
    aylo::render::vulkan::rendering::setCamera(uniforms, glm::mat4{1.0F},
                                               glm::ortho(-1.0F, 1.0F, -1.0F, 1.0F, 0.1F, 10.0F), false);
    EXPECT_FLOAT_EQ(uniforms.cameraPosition.w, 0.0F);
}
