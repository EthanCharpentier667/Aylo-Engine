/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Camera
*/

#pragma once

#include <cstdint>

#include "aylo/contract/renderer/view/View.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"

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
}  // namespace aylo::plugins::graphics
