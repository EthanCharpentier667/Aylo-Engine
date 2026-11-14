/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TransformMath
*/

#include "TransformMath.hpp"

#include <cmath>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Transform.hpp"

namespace aylo::plugins::transform {
namespace {
/// @return @p vector turned by @p angle radians, clockwise on screen (Y down).
[[nodiscard]] glm::vec2 rotate(glm::vec2 vector, float angle) noexcept {
    const float cos = std::cos(angle);
    const float sin = std::sin(angle);
    return glm::vec2{(vector.x * cos) - (vector.y * sin), (vector.x * sin) + (vector.y * cos)};
}

/// @return -1 if @p scale mirrors, on one axis only, which reverses the rotations under it; 1 otherwise.
[[nodiscard]] float rotationDirection(glm::vec2 scale) noexcept { return scale.x * scale.y < 0.0F ? -1.0F : 1.0F; }

/// @return @p value divided by @p scale, 0 on an axis @p scale flattens.
[[nodiscard]] glm::vec2 divide(glm::vec2 value, glm::vec2 scale) noexcept {
    return glm::vec2{scale.x != 0.0F ? value.x / scale.x : 0.0F, scale.y != 0.0F ? value.y / scale.y : 0.0F};
}

/// @return @p value divided by @p scale, 0 on an axis @p scale flattens.
[[nodiscard]] glm::vec3 divide(glm::vec3 value, glm::vec3 scale) noexcept {
    return glm::vec3{scale.x != 0.0F ? value.x / scale.x : 0.0F, scale.y != 0.0F ? value.y / scale.y : 0.0F,
                     scale.z != 0.0F ? value.z / scale.z : 0.0F};
}

/// @return The quaternion a reflected rotation (x, y, z, w) holds; glm takes w first.
[[nodiscard]] glm::quat toQuat(glm::vec4 rotation) noexcept {
    return glm::quat{rotation.w, rotation.x, rotation.y, rotation.z};
}

/// @return @p rotation as reflected: x, y, z, w, made unit length (the identity for a zero quaternion).
[[nodiscard]] glm::vec4 fromQuat(glm::quat rotation) noexcept {
    const float length = glm::length(rotation);
    if (length == 0.0F) {
        return glm::vec4{0.0F, 0.0F, 0.0F, 1.0F};
    }
    rotation /= length;
    return glm::vec4{rotation.x, rotation.y, rotation.z, rotation.w};
}
}  // namespace

GlobalTransform2D combine(const GlobalTransform2D& parent, const Transform2D& local) noexcept {
    return GlobalTransform2D{.position = parent.position + rotate(local.position * parent.scale, parent.rotation),
                             .rotation = parent.rotation + (rotationDirection(parent.scale) * local.rotation),
                             .scale = parent.scale * local.scale};
}

Transform2D toLocal(const GlobalTransform2D& parent, const GlobalTransform2D& world) noexcept {
    return Transform2D{.position = divide(rotate(world.position - parent.position, -parent.rotation), parent.scale),
                       .rotation = rotationDirection(parent.scale) * (world.rotation - parent.rotation),
                       .scale = divide(world.scale, parent.scale)};
}
Transform2D interpolate(const Transform2D& from, const Transform2D& to, float alpha) noexcept {
    const float turn = std::remainder(to.rotation - from.rotation, 2.0F * glm::pi<float>());
    return Transform2D{.position = from.position + ((to.position - from.position) * alpha),
                       .rotation = from.rotation + (turn * alpha),
                       .scale = from.scale + ((to.scale - from.scale) * alpha)};
}

GlobalTransform3D combine(const GlobalTransform3D& parent, const Transform3D& local) noexcept {
    const glm::quat rotation = toQuat(parent.rotation);
    return GlobalTransform3D{.position = parent.position + (rotation * (parent.scale * local.position)),
                             .rotation = fromQuat(rotation * toQuat(local.rotation)),
                             .scale = parent.scale * local.scale};
}

Transform3D toLocal(const GlobalTransform3D& parent, const GlobalTransform3D& world) noexcept {
    const glm::quat inverse = glm::inverse(toQuat(parent.rotation));
    return Transform3D{.position = divide(inverse * (world.position - parent.position), parent.scale),
                       .rotation = fromQuat(inverse * toQuat(world.rotation)),
                       .scale = divide(world.scale, parent.scale)};
}

Transform3D interpolate(const Transform3D& from, const Transform3D& to, float alpha) noexcept {
    return Transform3D{.position = from.position + ((to.position - from.position) * alpha),
                       .rotation = fromQuat(glm::slerp(toQuat(from.rotation), toQuat(to.rotation), alpha)),
                       .scale = from.scale + ((to.scale - from.scale) * alpha)};
}
glm::mat4 toMatrix(const GlobalTransform3D& global) noexcept {
    return glm::translate(glm::mat4{1.0F}, global.position) * glm::mat4_cast(glm::normalize(toQuat(global.rotation))) *
           glm::scale(glm::mat4{1.0F}, global.scale);
}
}  // namespace aylo::plugins::transform
