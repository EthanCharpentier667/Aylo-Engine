/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CameraView3D
*/

#include "aylo/contract/renderer/view/CameraView3D.hpp"

#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/matrix.hpp>
#include <optional>
#include <variant>

#include "aylo/contract/renderer/view/CameraView.hpp"
#include "aylo/contract/renderer/view/View.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"

namespace aylo::contract::renderer {
namespace {
/// @return The projection of @p projection for a viewport of @p aspect (width over height), in OpenGL's clip space
/// (Y up, depth -1 to 1).
glm::mat4 openGLProjectionOf(const Projection3D& projection, float aspect) {
    if (const auto* perspective = std::get_if<Perspective>(&projection)) {
        return glm::perspectiveRH_NO(perspective->fieldOfView, aspect, perspective->nearPlane, perspective->farPlane);
    }
    const auto& orthographic = std::get<Orthographic3D>(projection);
    const float halfHeight = orthographic.height / 2.0F;
    const float halfWidth = halfHeight * aspect;
    return glm::orthoRH_NO(-halfWidth, halfWidth, -halfHeight, halfHeight, orthographic.nearPlane,
                           orthographic.farPlane);
}

/// @return The projection of @p projection for a viewport of @p aspect (width over height), in Vulkan's clip space.
glm::mat4 projectionOf(const Projection3D& projection, float aspect) {
    glm::mat4 matrix{1.0F};
    if (const auto* perspective = std::get_if<Perspective>(&projection)) {
        matrix = glm::perspectiveRH_ZO(perspective->fieldOfView, aspect, perspective->nearPlane, perspective->farPlane);
    } else {
        const auto& orthographic = std::get<Orthographic3D>(projection);
        const float halfHeight = orthographic.height / 2.0F;
        const float halfWidth = halfHeight * aspect;
        matrix = glm::orthoRH_ZO(-halfWidth, halfWidth, -halfHeight, halfHeight, orthographic.nearPlane,
                                 orthographic.farPlane);
    }
    // Vulkan's clip space has Y down: the world's up must go to the top of the screen.
    return glm::scale(glm::mat4{1.0F}, glm::vec3{1.0F, -1.0F, 1.0F}) * matrix;
}
}  // namespace

CameraView3D::CameraView3D(const View3D& view, const glm::vec2 viewSize, const glm::uvec2 framebufferSize)
    : _viewport{CameraView{View{}, viewSize, framebufferSize}.getViewport()} {
    const glm::quat rotation{view.rotation.w, view.rotation.x, view.rotation.y, view.rotation.z};
    _view = glm::mat4_cast(glm::conjugate(glm::normalize(rotation))) * glm::translate(glm::mat4{1.0F}, -view.position);
    const float aspect = _viewport.size.y > 0.0F ? _viewport.size.x / _viewport.size.y : 1.0F;
    _projection = projectionOf(view.projection, aspect);
    _openGLProjection = openGLProjectionOf(view.projection, aspect);
    _viewProjection = _projection * _view;
    _inverse = glm::inverse(_viewProjection);
}

Ray3D CameraView3D::screenToRay(const glm::vec2 screen) const {
    const glm::vec2 size = glm::max(_viewport.size, glm::vec2{1.0F});
    const glm::vec2 ndc = (((screen - _viewport.position) / size) * 2.0F) - 1.0F;
    glm::vec4 nearPoint = _inverse * glm::vec4{ndc, 0.0F, 1.0F};
    glm::vec4 farPoint = _inverse * glm::vec4{ndc, 1.0F, 1.0F};
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;
    return Ray3D{.origin = glm::vec3{nearPoint}, .direction = glm::normalize(glm::vec3{farPoint - nearPoint})};
}

std::optional<glm::vec2> CameraView3D::worldToScreen(const glm::vec3 world) const {
    const glm::vec4 clip = _viewProjection * glm::vec4{world, 1.0F};
    if (clip.w <= 0.0F) {
        return std::nullopt;
    }
    const glm::vec2 ndc = glm::vec2{clip} / clip.w;
    return _viewport.position + (((ndc + 1.0F) / 2.0F) * _viewport.size);
}

}  // namespace aylo::contract::renderer
