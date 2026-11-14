/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PhysicsDebugShapes
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <vector>

#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Lines3D.hpp"
#include "aylo/contract/renderer/drawing/Shape.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/Collider3D.hpp"
#include "plugins/physics/debug/Export.hpp"
#include "plugins/transform/Transform.hpp"

namespace aylo::plugins::physics::debug {

/// @return The outline of a collider where its entity is: a rectangle, circle or capsule turned with the entity and
/// moved by the collider's offset, in world units. A sensor is drawn at half the colour's opacity.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_DEBUG_API contract::renderer::Shape debugColliderShape(
    const Collider& collider, const transform::GlobalTransform2D& transform, contract::renderer::Color color,
    float thickness) noexcept;

/// @return An arrow from @p from along @p velocity, as long as the distance covered in @p scale seconds: its shaft and
/// the two strokes of its head, as lines. Nothing for a speed too small to draw.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_DEBUG_API std::vector<contract::renderer::Shape> debugVelocityArrow(
    glm::vec2 from, glm::vec2 velocity, float scale, contract::renderer::Color color, float thickness);

/// @return The wireframe of a 3D collider where its entity is, in metres: a box's 12 edges, a sphere's three great
/// circles, a capsule's two end circles, four sides and the arcs of its caps, turned with the entity (not scaled) and
/// moved by the collider's offset. A sensor is drawn at half the colour's opacity.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_DEBUG_API contract::renderer::Lines3D debugCollider3DLines(
    const Collider3D& collider, const transform::GlobalTransform3D& transform, contract::renderer::Color color);

/// @return An arrow from @p from along @p velocity, as long as the distance covered in @p scale seconds, in the 3D
/// world: its shaft and two strokes of its head. No points for a speed too small to draw.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_DEBUG_API contract::renderer::Lines3D debugVelocityArrow3D(
    const glm::vec3& from, const glm::vec3& velocity, float scale, contract::renderer::Color color);

}  // namespace aylo::plugins::physics::debug
