/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BodySettings
*/

#pragma once

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include <cstdint>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/Collider3D.hpp"
#include "plugins/physics/Export.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/transform/Transform.hpp"

namespace aylo::plugins::physics::detail {

/// @brief Builds a collider's Jolt shape: a box, a sphere (a circle in the XY plane) or a capsule along Y, moved by its
/// offset. Distances are divided by unitsPerMeter, and the offset's Y is flipped.
/// @throws exceptions::PhysicsException When the shape's size, radius or half height is not positive.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API JPH::RefConst<JPH::Shape> makeShape(const Collider& collider,
                                                                          float unitsPerMeter);

/// @brief Builds the creation settings of an entity's body, locked to the XY plane unless static.
/// @param collider The entity's shape and material.
/// @param body How it moves, or nullptr for a static body.
/// @param transform Where it is, in world units, Y down.
/// @param unitsPerMeter World units in one of Jolt's meters.
/// @param userData Stored on the body to find the entity back (Entity::getBits()).
/// @throws exceptions::PhysicsException When the shape is invalid, a dynamic body's mass is not positive, or the
/// collider's layer or mask does not fit (layers::isValid()).
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API JPH::BodyCreationSettings makeBodySettings(
    const Collider& collider, const RigidBody* body, const transform::Transform2D& transform, float unitsPerMeter,
    std::uint64_t userData);

/// @brief Builds a 3D collider's Jolt shape: a box, a sphere or a capsule along Y, moved by its offset. In metres.
/// @throws exceptions::PhysicsException When the shape's size, radius or half height is not positive.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API JPH::RefConst<JPH::Shape> makeShape3D(const Collider3D& collider);

/// @brief Builds the creation settings of a 3D entity's body, free to move and turn along every axis.
/// @param collider The entity's shape and material.
/// @param body How it moves, or nullptr for a static body.
/// @param transform Where it is, in metres, Y up.
/// @param userData Stored on the body to find the entity back (Entity::getBits()).
/// @throws exceptions::PhysicsException As makeBodySettings().
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API JPH::BodyCreationSettings makeBodySettings3D(
    const Collider3D& collider, const RigidBody* body, const transform::Transform3D& transform, std::uint64_t userData);

}  // namespace aylo::plugins::physics::detail
