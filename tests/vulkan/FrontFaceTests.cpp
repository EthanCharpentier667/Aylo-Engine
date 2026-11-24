/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FrontFaceTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

#include "aylo/contract/renderer/view/CameraView3D.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"

namespace {
using Triangle = std::array<glm::vec3, 3>;

/// @return The signed area Vulkan gives @p triangle seen through @p view in a 800x600 framebuffer (spec, "Basic Polygon
/// Rasterization"): -1/2 * sum(x_i * y_(i+1) - x_(i+1) * y_i) in framebuffer coordinates, after the viewport the
/// renderer sets (positive height). Positive: front facing under VK_FRONT_FACE_COUNTER_CLOCKWISE, the pipelines'
/// default.
float vulkanArea(const aylo::contract::renderer::View3D& view, const Triangle& triangle) {
    const glm::vec2 size{800.0F, 600.0F};
    const aylo::contract::renderer::CameraView3D shown{view, glm::vec2{0.0F}, glm::uvec2{800, 600}};
    std::array<glm::vec2, 3> screen{};
    for (std::size_t index = 0; index < triangle.size(); ++index) {
        const glm::vec4 clip = shown.getViewProjection() * glm::vec4{triangle.at(index), 1.0F};
        const glm::vec2 ndc = glm::vec2{clip} / clip.w;
        screen.at(index) = (ndc * 0.5F + 0.5F) * size;
    }
    float sum = 0.0F;
    for (std::size_t index = 0; index < screen.size(); ++index) {
        const glm::vec2 here = screen.at(index);
        const glm::vec2 next = screen.at((index + 1) % screen.size());
        sum += (here.x * next.y) - (next.x * here.y);
    }
    return -0.5F * sum;
}

/// @brief A triangle in the plane z = -5, counter-clockwise seen from the origin (its front faces +Z, toward it).
constexpr Triangle kFacingTheOrigin{glm::vec3{-1.0F, -1.0F, -5.0F}, glm::vec3{1.0F, -1.0F, -5.0F},
                                    glm::vec3{0.0F, 1.0F, -5.0F}};
}  // namespace

TEST(FrontFace, ATriangleCounterClockwiseInTheWorldIsFrontFacingInVulkan) {
    // The camera at the origin looks along -Z: the triangle's front faces it.
    EXPECT_GT(vulkanArea(aylo::contract::renderer::View3D{}, kFacingTheOrigin), 0.0F);
    const aylo::contract::renderer::View3D orthographic{.projection = aylo::contract::renderer::Orthographic3D{}};
    EXPECT_GT(vulkanArea(orthographic, kFacingTheOrigin), 0.0F);
}

TEST(FrontFace, ItsBackIsSeenFromBehind) {
    // From z = -10 looking along +Z (turned half a turn around Y), the same triangle shows its back.
    const aylo::contract::renderer::View3D behind{.position = glm::vec3{0.0F, 0.0F, -10.0F},
                                                  .rotation = glm::vec4{0.0F, 1.0F, 0.0F, 0.0F}};
    EXPECT_LT(vulkanArea(behind, kFacingTheOrigin), 0.0F);
}
