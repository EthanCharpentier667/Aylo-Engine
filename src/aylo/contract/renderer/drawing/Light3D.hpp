/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Light3D
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

namespace aylo::contract::renderer {

/// @brief What shape a light of the 3D world has.
enum class LightKind3D : std::uint8_t {
    kDirectional,  ///< From infinitely far along a direction: the sun. Its intensity is in lux (W/m²).
    kPoint,        ///< From a point, all around: a bulb. Its intensity is a power, in watts.
    kSpot,         ///< From a point, in a cone: a torch. A power in watts, as a point light's.
    kArea,         ///< From a rectangle, on its front side: a window, a screen, a softbox. A power in watts.
};

/// @brief Most lights a frame draws with (IRenderer::setLights3D()): past it, the renderer keeps the directional lights
/// and the nearest others to the camera.
inline constexpr std::size_t kMaxLights3D = 64;

/// @brief One light of the 3D world, in the world's space, given to IRenderer::setLights3D(). Physical units, as glTF's
/// KHR_lights_punctual and Blender use them, so a scene exported from a 3D tool keeps its balance.
struct Light3D {
    LightKind3D kind{LightKind3D::kPoint};   ///< Its shape.
    glm::vec3 position{0.0F};                ///< Where it is (point, spot, the centre of an area).
    glm::vec3 direction{0.0F, 0.0F, -1.0F};  ///< Where it shines, unit (directional, spot, area).
    glm::vec3 right{1.0F, 0.0F, 0.0F};       ///< The area's width axis, unit, across direction.
    glm::vec3 color{1.0F};                   ///< Linear RGB, not multiplied by the intensity.
    float intensity{1000.0F};                ///< Watts (point, spot, area) or lux (directional).
    float range{0.0F};                       ///< Distance past which it lights nothing; 0: no limit.
    float innerCone{0.0F};                   ///< Spot: the half-angle lit fully, radians.
    float outerCone{0.7853982F};             ///< Spot: the half-angle past which nothing is lit, radians.
    glm::vec2 size{1.0F};                    ///< Area: its width and height, in world units.
    bool castShadows{false};                 ///< Whether it casts shadows (R3D-08).

    [[nodiscard]] bool operator==(const Light3D& other) const = default;
};

/// @brief The light that reaches everything of the 3D world, wherever it is: what is not lit by a Light3D is not black.
struct Environment3D {
    glm::vec3 ambient{0.0F};  ///< Linear RGB, multiplied by strength.
    float strength{1.0F};     ///< How much of ambient reaches the surfaces.

    [[nodiscard]] bool operator==(const Environment3D& other) const = default;
};

}  // namespace aylo::contract::renderer
