/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** View3D
*/

#pragma once

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <variant>

namespace aylo::contract::renderer {

/// @brief A perspective projection: what is farther looks smaller.
struct Perspective {
    float fieldOfView = 1.0471976F;  ///< The vertical angle the view shows, in radians (60 degrees).
    float nearPlane = 0.1F;          ///< Distance of the nearest thing drawn, in world units; above 0.
    float farPlane = 1000.0F;        ///< Distance of the farthest thing drawn; above nearPlane.
};

/// @brief An orthographic projection of the 3D world: sizes do not change with the distance (an isometric view, a
/// map seen from above).
struct Orthographic3D {
    float height = 10.0F;      ///< How many world units the view shows from its bottom to its top.
    float nearPlane = 0.1F;    ///< Distance of the nearest thing drawn, in world units.
    float farPlane = 1000.0F;  ///< Distance of the farthest thing drawn; above nearPlane.
};

/// @brief How a View3D projects the 3D world onto the screen.
using Projection3D = std::variant<Perspective, Orthographic3D>;

/// @brief The point of view the 3D world of a frame is seen from, set with IRenderer::setView3D().
/// @details The 3D world is right-handed, Y up; the camera looks along its -Z. In an App, the graphics plugin builds it
/// from the Camera3D component with the highest priority and the GlobalTransform3D of its entity.
struct View3D {
    glm::vec3 position{0.0F};                    ///< Where the camera is, in world units.
    glm::vec4 rotation{0.0F, 0.0F, 0.0F, 1.0F};  ///< Its orientation, a unit quaternion (x, y, z, w).
    Projection3D projection = Perspective{};     ///< How the world is projected.
};

/// @brief A half-line in the 3D world: what a point of the screen sees (CameraView3D::screenToRay()).
struct Ray3D {
    glm::vec3 origin{0.0F};     ///< Where it starts: on the near plane.
    glm::vec3 direction{0.0F};  ///< Where it goes, of length 1.
};

}  // namespace aylo::contract::renderer
