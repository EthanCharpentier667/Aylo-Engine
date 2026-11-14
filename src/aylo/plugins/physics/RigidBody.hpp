/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RigidBody
*/

#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"

namespace aylo::plugins::physics {

/// @brief How a body moves.
enum class BodyType : std::uint8_t {
    kStatic,     ///< Never moves (walls, ground).
    kKinematic,  ///< Moved by gameplay through its Transform2D, pushes dynamic bodies.
    kDynamic,    ///< Moved by the simulation: gravity, forces, collisions.
};

}  // namespace aylo::plugins::physics

// The ECS reflects scalars only: a BodyType field is reflected as its underlying byte. The specialization must sit
// outside the physics namespace, in one that encloses aylo::ecs.
/// @brief A BodyType field is reflected as kU8.
template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::physics::BodyType>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a BodyType field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::plugins::physics::BodyType> {
    static constexpr std::array<std::string_view, 3> kNames{"static", "kinematic", "dynamic"};
};
static_assert(sizeof(aylo::plugins::physics::BodyType) == 1, "BodyType must match FieldType::kU8");

namespace aylo::plugins::physics {

/// @brief How an entity moves in the physics world: gameplay writes it, the physics plugin builds the body from it.
/// @details Plain data, no Jolt: it can be reflected, replicated and written from Luau. The entity also needs a
/// Collider to get a body; a Collider without a RigidBody is a static body.
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform2D{.position = {100.0F, 200.0F}})
///     .add(aylo::plugins::physics::RigidBody{.type = aylo::plugins::physics::BodyType::kDynamic})
///     .add(aylo::plugins::physics::Collider{.shape = aylo::plugins::physics::ShapeType::kCircle, .radius = 8.0F});
/// @endcode
struct RigidBody {
    BodyType type = BodyType::kDynamic;  ///< How the body moves.
    float mass = 1.0F;                   ///< In kilograms; only dynamic bodies use it.
    float linearDamping = 0.05F;         ///< Slows the linear velocity down over time, per second.
    float angularDamping = 0.05F;        ///< Slows the angular velocity down over time, per second.
    float gravityScale = 1.0F;           ///< Multiplies the world's gravity for this body: 0 ignores it.
    bool fixedRotation = false;          ///< Never rotates (a ship that must not spin on impact).
    bool continuousCollision = false;    ///< Never tunnels through thin colliders when fast (bullets), at a cost.
};
AYLO_ECS_COMPONENT(RigidBody, "aylo.RigidBody", type, mass, linearDamping, angularDamping, gravityScale, fixedRotation,
                   continuousCollision);

}  // namespace aylo::plugins::physics
