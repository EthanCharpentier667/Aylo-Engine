/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Camera
*/

#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "aylo/contract/renderer/view/View.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"

namespace aylo::plugins::graphics {

/// @brief The component ("aylo.Camera2D") that makes an entity a point of view: the graphics plugin shows the world
/// from the GlobalTransform2D of the camera entity with the highest priority.
/// @details Where the camera is comes from the entity's Transform2D, as for any other entity: an entity with a Camera2D
/// but no Transform2D is never shown.
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform2D{.position = {960.0F, 540.0F}})
///     .add(aylo::plugins::graphics::Camera2D{});
/// @endcode
/// @note It holds an Orthographic, not the Projection variant: a std::variant is not standard-layout on every standard
/// library, and a component with reflected fields must be. A Perspective projection will need its own field or
/// component.
struct Camera2D {
    contract::renderer::Orthographic projection;  ///< How the world is projected.
    std::int32_t priority = 0;                    ///< Among several camera entities, the highest one is shown.
};

AYLO_ECS_COMPONENT(Camera2D, "aylo.Camera2D", priority);

/// @brief How a Camera3D projects the 3D world.
enum class CameraProjection : std::uint8_t {
    kPerspective,   ///< What is farther looks smaller.
    kOrthographic,  ///< Sizes do not change with the distance.
};
}  // namespace aylo::plugins::graphics

/// @brief A CameraProjection field is reflected as kU8, with the names scripts write.
template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::graphics::CameraProjection>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a CameraProjection field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::plugins::graphics::CameraProjection> {
    static constexpr std::array<std::string_view, 2> kNames{"perspective", "orthographic"};
};

namespace aylo::plugins::graphics {

/// @brief The component ("aylo.Camera3D") that makes a 3D entity the point of view of the 3D world: the graphics plugin
/// shows it from the GlobalTransform3D of the camera entity with the highest priority, looking along its -Z.
/// @details Independent of Camera2D: the 2D world keeps its own camera. A renderer that cannot draw 3D (supports3D())
/// shows no 3D world, and the plugin says so once.
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform3D{.position = {0.0F, 2.0F, 6.0F}})
///     .add(aylo::plugins::graphics::Camera3D{});
/// @endcode
struct Camera3D {
    CameraProjection projection = CameraProjection::kPerspective;  ///< Perspective or orthographic.
    float fieldOfView = 60.0F;         ///< Perspective: the vertical angle shown, in degrees, above 0 and below 180.
    float orthographicHeight = 10.0F;  ///< Orthographic: the world units shown from the bottom to the top.
    float nearPlane = 0.1F;            ///< Distance of the nearest thing drawn, in world units; above 0.
    float farPlane = 1000.0F;          ///< Distance of the farthest thing drawn; above nearPlane.
    std::int32_t priority = 0;         ///< Among several 3D camera entities, the highest one is shown.
};

AYLO_ECS_COMPONENT(Camera3D, "aylo.Camera3D", projection, fieldOfView, orthographicHeight, nearPlane, farPlane,
                   priority);
}  // namespace aylo::plugins::graphics
