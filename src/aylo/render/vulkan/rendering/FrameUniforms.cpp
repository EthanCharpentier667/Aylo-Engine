/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FrameUniforms
*/

#include "FrameUniforms.hpp"

#include <cmath>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/matrix.hpp>

namespace aylo::render::vulkan::rendering {

namespace {

/// @return The inverse of @p matrix, or identity when it has none.
glm::mat4 inverseOrIdentity(const glm::mat4& matrix) noexcept {
    constexpr float kMinDeterminant = 1e-20F;
    return std::abs(glm::determinant(matrix)) > kMinDeterminant ? glm::inverse(matrix) : glm::mat4{1.0F};
}

}  // namespace

void setCamera(FrameUniforms& uniforms, const glm::mat4& view, const glm::mat4& projection, bool perspective) noexcept {
    uniforms.view = view;
    uniforms.projection = projection;
    uniforms.viewProjection = projection * view;
    uniforms.inverseView = inverseOrIdentity(view);
    uniforms.inverseProjection = inverseOrIdentity(projection);
    uniforms.cameraPosition = glm::vec4{glm::vec3{glm::column(uniforms.inverseView, 3)}, perspective ? 1.0F : 0.0F};
}

}  // namespace aylo::render::vulkan::rendering
