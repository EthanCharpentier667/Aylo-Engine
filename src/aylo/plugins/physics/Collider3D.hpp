/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Collider3D
*/

#pragma once

#include <array>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <string_view>
#include <type_traits>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"
#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "plugins/physics/Collider.hpp"

namespace aylo::plugins::physics {

/// @brief The shape of a 3D collider.
enum class Shape3DType : std::uint8_t {
    kBox,      ///< A box of Collider3D::size.
    kSphere,   ///< A sphere of Collider3D::radius.
    kCapsule,  ///< A capsule along Y: Collider3D::radius, and Collider3D::halfHeight for its straight part.
};

}  // namespace aylo::plugins::physics

/// @brief A Shape3DType field is reflected as kU8.
template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::physics::Shape3DType>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a Shape3DType field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::plugins::physics::Shape3DType> {
    static constexpr std::array<std::string_view, 3> kNames{"box", "sphere", "capsule"};
};
static_assert(sizeof(aylo::plugins::physics::Shape3DType) == 1, "Shape3DType must match FieldType::kU8");

namespace aylo::plugins::physics {

/// @brief The shape and material a 3D entity collides with: the 3D family's Collider.
/// @details Placed by the entity's Transform3D, in metres (no PhysicsSettings::unitsPerMeter), Y up. Each shape reads
/// its own fields: size for a box, radius for a sphere, radius and halfHeight for a capsule. Without a RigidBody, the
/// entity is a static body. Layers and masks work as for Collider. A 3D body never touches a 2D one, and an entity
/// has one body: give it a Collider or a Collider3D, not both (the Collider3D wins).
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform3D{.position = {0.0F, 5.0F, 0.0F}})
///     .add(aylo::plugins::physics::Collider3D{.shape = aylo::plugins::physics::Shape3DType::kSphere})
///     .add(aylo::plugins::physics::RigidBody{});
/// @endcode
struct Collider3D {
    Shape3DType shape = Shape3DType::kBox;  ///< Which fields below describe the shape.
    glm::vec3 size{1.0F, 1.0F, 1.0F};       ///< kBox: full width, height and depth, in metres.
    float radius = 0.5F;                    ///< kSphere and kCapsule: radius, in metres.
    float halfHeight = 0.5F;                ///< kCapsule: half the length of the straight part, along Y, in metres.
    glm::vec3 offset{0.0F, 0.0F, 0.0F};     ///< The shape's centre relative to the Transform3D, in its axes, metres.
    bool sensor = false;                    ///< Detects overlaps (triggers) without colliding.
    float friction = 0.2F;                  ///< How much the surface resists sliding, usually in [0, 1].
    float restitution = 0.0F;               ///< How bouncy the surface is: 0 absorbs, 1 bounces back fully.
    std::uint32_t layer = 1U << 0;          ///< The collision categories it belongs to, usually one bit.
    std::uint32_t mask = kAllLayers;        ///< The collision categories it collides with.
};
AYLO_ECS_COMPONENT(Collider3D, "aylo.Collider3D", shape, size, radius, halfHeight, offset, sensor, friction,
                   restitution, layer, mask);

}  // namespace aylo::plugins::physics
