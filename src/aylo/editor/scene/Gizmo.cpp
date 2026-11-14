/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Gizmo
*/

#include "editor/scene/Gizmo.hpp"

#include <algorithm>
#include <cstdint>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <optional>
#include <span>

#include "plugins/transform/Transform.hpp"

namespace aylo::editor::scene {
namespace {
constexpr float kTurnPerPixel = 0.01F;  ///< Radians a pixel of drag turns.
constexpr float kMinScale = 0.01F;      ///< The smallest scale a drag leaves.

/// @return The distance from @p point to the segment @p from - @p to.
float distanceToSegment(glm::vec2 point, glm::vec2 from, glm::vec2 to) {
    const glm::vec2 along = to - from;
    const float length = glm::dot(along, along);
    const float share = length > 0.0F ? std::clamp(glm::dot(point - from, along) / length, 0.0F, 1.0F) : 0.0F;
    return glm::length(point - (from + (along * share)));
}

/// @return The world axis @p axis (0 X, 1 Y, 2 Z).
glm::vec3 axisOf(std::uint8_t axis) {
    switch (axis) {
        case 0:
            return {1.0F, 0.0F, 0.0F};
        case 1:
            return {0.0F, 1.0F, 0.0F};
        default:
            return {0.0F, 0.0F, 1.0F};
    }
}

/// @return The component @p axis of @p value (0 X, 1 Y, else Z).
float& componentOf(glm::vec3& value, std::uint8_t axis) {
    if (axis == 0) {
        return value.x;
    }
    return axis == 1 ? value.y : value.z;
}

/// @return The component @p axis of @p value (0 X, else Y).
float& componentOf(glm::vec2& value, std::uint8_t axis) { return axis == 0 ? value.x : value.y; }

/// @return How far along the handle, in handle lengths, a mouse move of @p mouse goes; and across it, in pixels.
glm::vec2 along(const GizmoHandle& handle, glm::vec2 mouse) {
    const glm::vec2 screen = handle.to - handle.from;
    const float length = glm::length(screen);
    if (!(length > 0.0F)) {
        return glm::vec2{0.0F};
    }
    const glm::vec2 unit = screen / length;
    return {glm::dot(mouse, unit) / length, (unit.x * mouse.y) - (unit.y * mouse.x)};
}
}  // namespace

std::optional<std::uint8_t> hitGizmo(std::span<const GizmoHandle> handles, glm::vec2 mouse, float tolerance) {
    std::optional<std::uint8_t> best;
    float nearest = tolerance;
    for (const GizmoHandle& handle : handles) {
        const float distance = distanceToSegment(mouse, handle.from, handle.to);
        if (distance <= nearest) {
            nearest = distance;
            best = handle.axis;
        }
    }
    return best;
}

plugins::transform::Transform3D dragGizmo(GizmoTool tool, const GizmoHandle& handle,
                                          const plugins::transform::Transform3D& start, glm::vec2 mouse) {
    plugins::transform::Transform3D result = start;
    const glm::vec2 moved = along(handle, mouse);
    const glm::vec3 axis = axisOf(handle.axis);
    switch (tool) {
        case GizmoTool::kMove:
            result.position += axis * (moved.x * handle.worldLength);
            break;
        case GizmoTool::kRotate: {
            const glm::quat turn = glm::angleAxis(moved.y * kTurnPerPixel, axis);
            const glm::quat from{start.rotation.w, start.rotation.x, start.rotation.y, start.rotation.z};
            const glm::quat to = glm::normalize(turn * from);
            result.rotation = {to.x, to.y, to.z, to.w};
            break;
        }
        case GizmoTool::kScale: {
            glm::vec3 from = start.scale;
            componentOf(result.scale, handle.axis) =
                std::max(kMinScale, componentOf(from, handle.axis) * (1.0F + moved.x));
            break;
        }
    }
    return result;
}

plugins::transform::Transform2D dragGizmo2D(GizmoTool tool, const GizmoHandle& handle,
                                            const plugins::transform::Transform2D& start, glm::vec2 mouse) {
    plugins::transform::Transform2D result = start;
    const glm::vec2 moved = along(handle, mouse);
    switch (tool) {
        case GizmoTool::kMove:
            componentOf(result.position, handle.axis) += moved.x * handle.worldLength;
            break;
        case GizmoTool::kRotate:
            result.rotation = start.rotation + ((mouse.x + mouse.y) * kTurnPerPixel);
            break;
        case GizmoTool::kScale: {
            glm::vec2 from = start.scale;
            componentOf(result.scale, handle.axis) =
                std::max(kMinScale, componentOf(from, handle.axis) * (1.0F + moved.x));
            break;
        }
    }
    return result;
}

}  // namespace aylo::editor::scene
