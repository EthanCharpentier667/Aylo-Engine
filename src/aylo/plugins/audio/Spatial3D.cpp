/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Spatial3D
*/

#include "plugins/audio/Spatial3D.hpp"

#include <algorithm>
#include <cmath>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>

namespace aylo::plugins::audio {
namespace {
/// @brief The shortest distance the math reads, in metres: closer is at the listener's place.
constexpr float kNear = 1e-2F;

/// @return The listener's rotation as a unit quaternion; the identity for a zero or invalid one.
[[nodiscard]] glm::quat rotationOf(const ListenerPose3D& listener) noexcept {
    const glm::quat rotation{listener.rotation.w, listener.rotation.x, listener.rotation.y, listener.rotation.z};
    const float length = glm::length(rotation);
    if (!(length > 0.0F) || !std::isfinite(length)) {
        return glm::quat{1.0F, 0.0F, 0.0F, 0.0F};
    }
    return rotation / length;
}
}  // namespace

Spatial3D spatialize(const SoundPlace3D& place, const ListenerPose3D& listener) noexcept {
    const glm::vec3 offset = place.position - listener.position;
    const float distance = glm::length(offset);
    if (!(distance > kNear) || !std::isfinite(distance)) {
        return Spatial3D{};
    }
    // In the listener's axes: +X on its right.
    const glm::vec3 local = glm::inverse(rotationOf(listener)) * offset;
    const float minDistance = std::max(place.minDistance, kNear);
    const float maxDistance = std::max(place.maxDistance, minDistance);
    return Spatial3D{.pan = std::clamp(local.x / distance, -1.0F, 1.0F),
                     .gain = minDistance / std::clamp(distance, minDistance, maxDistance)};
}

}  // namespace aylo::plugins::audio
