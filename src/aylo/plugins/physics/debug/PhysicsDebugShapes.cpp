/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PhysicsDebugShapes
*/

#include "plugins/physics/debug/PhysicsDebugShapes.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <vector>

#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Lines3D.hpp"
#include "aylo/contract/renderer/drawing/Shape.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/Collider3D.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformMath.hpp"

namespace aylo::plugins::physics::debug {
namespace {
constexpr float kMinArrowLength = 0.5F;     ///< Shorter arrows are not drawn: a still body has none.
constexpr float kHeadLength = 8.0F;         ///< The arrow head's strokes, at most, in world units.
constexpr float kHeadShare = 0.3F;          ///< ...and at most this share of the arrow.
constexpr float kHeadCos = 0.866F;          ///< cos(30 degrees): the strokes' angle with the shaft.
constexpr float kHeadSin = 0.5F;            ///< sin(30 degrees).
constexpr float kSensorOpacity = 0.5F;      ///< A sensor's outline is fainter.
constexpr int kCircleSegments = 24;         ///< Segments of a full circle in 3D.
constexpr float kMinArrowLength3D = 0.01F;  ///< In metres: shorter 3D arrows are not drawn.
constexpr float kHeadLength3D = 0.25F;      ///< A 3D arrow head's strokes, at most, in metres.

/// @brief Appends the segments of an arc around @p center, from @p from to @p to radians, in the plane of the unit
/// vectors @p u and @p v.
void addArc(std::vector<glm::vec3>& points, const glm::vec3& center, const glm::vec3& u, const glm::vec3& v,
            float radius, float from, float to) {
    const auto segments = static_cast<int>(std::ceil(kCircleSegments * (to - from) / (2.0F * glm::pi<float>())));
    const auto at = [&](float angle) { return center + (radius * ((std::cos(angle) * u) + (std::sin(angle) * v))); };
    for (int index = 0; index < segments; ++index) {
        points.push_back(at(from + ((to - from) * static_cast<float>(index) / static_cast<float>(segments))));
        points.push_back(at(from + ((to - from) * static_cast<float>(index + 1) / static_cast<float>(segments))));
    }
}

/// @brief Appends the 12 edges of a box of half extents @p half around @p center.
void addBox(std::vector<glm::vec3>& points, const glm::vec3& center, const glm::vec3& half) {
    const auto corner = [&](int index) {
        return center + glm::vec3{(index & 1) != 0 ? half.x : -half.x, (index & 2) != 0 ? half.y : -half.y,
                                  (index & 4) != 0 ? half.z : -half.z};
    };
    for (int index = 0; index < 8; ++index) {
        for (const int axis : {1, 2, 4}) {
            if ((index & axis) == 0) {
                points.push_back(corner(index));
                points.push_back(corner(index | axis));
            }
        }
    }
}

/// @return @p local turned by @p rotation, clockwise on screen with Y down, as the renderer turns shapes.
glm::vec2 rotate(glm::vec2 local, float rotation) noexcept {
    const float c = std::cos(rotation);
    const float s = std::sin(rotation);
    return {(c * local.x) - (s * local.y), (s * local.x) + (c * local.y)};
}
}  // namespace

contract::renderer::Shape debugColliderShape(const Collider& collider, const transform::GlobalTransform2D& transform,
                                             contract::renderer::Color color, float thickness) noexcept {
    using contract::renderer::ShapeKind;
    if (collider.sensor) {
        color.w *= kSensorOpacity;
    }
    contract::renderer::Shape shape{
        .position = transform.position + rotate(collider.offset, transform.rotation),
        .rotation = transform.rotation,
        .fillColor = contract::renderer::Color{0.0F},
        .outlineColor = color,
        .outlineThickness = thickness,
    };
    switch (collider.shape) {
        case ShapeType::kBox:
            shape.kind = ShapeKind::kRect;
            shape.size = collider.size;
            break;
        case ShapeType::kCircle:
            shape.kind = ShapeKind::kCircle;
            shape.radius = collider.radius;
            break;
        case ShapeType::kCapsule:
            shape.kind = ShapeKind::kCapsule;
            shape.radius = collider.radius;
            shape.halfHeight = collider.halfHeight;
            break;
    }
    return shape;
}

std::vector<contract::renderer::Shape> debugVelocityArrow(glm::vec2 from, glm::vec2 velocity, float scale,
                                                          contract::renderer::Color color, float thickness) {
    using contract::renderer::Shape;
    using contract::renderer::ShapeKind;
    const glm::vec2 travel = velocity * scale;
    const float length = glm::length(travel);
    if (!(length >= kMinArrowLength)) {
        return {};
    }
    const glm::vec2 to = from + travel;
    const glm::vec2 direction = travel / length;
    const glm::vec2 side{-direction.y, direction.x};
    const float head = std::min(kHeadLength, length * kHeadShare);
    const auto stroke = [&](glm::vec2 start, glm::vec2 end) {
        return Shape{
            .kind = ShapeKind::kLine, .position = start, .end = end, .radius = thickness * 0.5F, .fillColor = color};
    };
    return {
        stroke(from, to),
        stroke(to, to - (((direction * kHeadCos) + (side * kHeadSin)) * head)),
        stroke(to, to - (((direction * kHeadCos) - (side * kHeadSin)) * head)),
    };
}

contract::renderer::Lines3D debugCollider3DLines(const Collider3D& collider,
                                                 const transform::GlobalTransform3D& transform,
                                                 contract::renderer::Color color) {
    if (collider.sensor) {
        color.w *= kSensorOpacity;
    }
    constexpr glm::vec3 kX{1.0F, 0.0F, 0.0F};
    constexpr glm::vec3 kY{0.0F, 1.0F, 0.0F};
    constexpr glm::vec3 kZ{0.0F, 0.0F, 1.0F};
    constexpr float kTurn = 2.0F * glm::pi<float>();
    constexpr float kHalfTurn = glm::pi<float>();
    contract::renderer::Lines3D lines{
        .color = color,
        .world = transform::toMatrix(
            transform::GlobalTransform3D{.position = transform.position, .rotation = transform.rotation}),
    };
    const glm::vec3& center = collider.offset;
    switch (collider.shape) {
        case Shape3DType::kBox:
            addBox(lines.points, center, collider.size * 0.5F);
            break;
        case Shape3DType::kSphere:
            addArc(lines.points, center, kX, kY, collider.radius, 0.0F, kTurn);
            addArc(lines.points, center, kX, kZ, collider.radius, 0.0F, kTurn);
            addArc(lines.points, center, kY, kZ, collider.radius, 0.0F, kTurn);
            break;
        case Shape3DType::kCapsule: {
            const glm::vec3 top = center + (kY * collider.halfHeight);
            const glm::vec3 bottom = center - (kY * collider.halfHeight);
            addArc(lines.points, top, kX, kZ, collider.radius, 0.0F, kTurn);
            addArc(lines.points, bottom, kX, kZ, collider.radius, 0.0F, kTurn);
            for (const glm::vec3& side : {kX, -kX, kZ, -kZ}) {
                lines.points.push_back(top + (side * collider.radius));
                lines.points.push_back(bottom + (side * collider.radius));
            }
            addArc(lines.points, top, kX, kY, collider.radius, 0.0F, kHalfTurn);
            addArc(lines.points, top, kZ, kY, collider.radius, 0.0F, kHalfTurn);
            addArc(lines.points, bottom, kX, kY, collider.radius, kHalfTurn, kTurn);
            addArc(lines.points, bottom, kZ, kY, collider.radius, kHalfTurn, kTurn);
            break;
        }
    }
    return lines;
}

contract::renderer::Lines3D debugVelocityArrow3D(const glm::vec3& from, const glm::vec3& velocity, float scale,
                                                 contract::renderer::Color color) {
    contract::renderer::Lines3D lines{.color = color};
    const glm::vec3 travel = velocity * scale;
    const float length = glm::length(travel);
    if (!(length >= kMinArrowLength3D)) {
        return lines;
    }
    const glm::vec3 to = from + travel;
    const glm::vec3 direction = travel / length;
    // Any vector across the shaft: the head is drawn in the plane of the shaft and it.
    const glm::vec3 helper = std::abs(direction.y) < 0.9F ? glm::vec3{0.0F, 1.0F, 0.0F} : glm::vec3{1.0F, 0.0F, 0.0F};
    const glm::vec3 side = glm::normalize(glm::cross(direction, helper));
    const float head = std::min(kHeadLength3D, length * kHeadShare);
    lines.points = {
        from, to,
        to,   to - (((direction * kHeadCos) + (side * kHeadSin)) * head),
        to,   to - (((direction * kHeadCos) - (side * kHeadSin)) * head),
    };
    return lines;
}

}  // namespace aylo::plugins::physics::debug
