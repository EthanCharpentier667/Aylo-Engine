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

#include "aylo/contract/renderer/drawing/Texture.hpp"

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
    float shadowSoftness{1.0F};    ///< 0: hard shadows; 1: as soft as a light of its size (the sun's half a degree, a
                                   ///< 10 cm bulb, an area light's rectangle); more: softer.
    float shadowBias{1.0F};        ///< Multiplies the depth bias that keeps a surface from shadowing itself (acne).
    float shadowNormalBias{1.0F};  ///< Texels the shadow's lookup moves along the surface's normal, for the same.

    [[nodiscard]] bool operator==(const Light3D& other) const = default;
};

/// @brief What is drawn behind the 3D world, where no model is.
enum class SkyKind : std::uint8_t {
    kNone,      ///< Nothing: the clear colour, and no light from the sky.
    kGradient,  ///< Three colours, from the top through the horizon to the bottom: no image.
    kTexture,   ///< An equirectangular image (twice as wide as high, the horizon across its middle), sRGB or HDR.
};

/// @brief The light that reaches everything of the 3D world, wherever it is: what is not lit by a Light3D is not black.
/// A sky is drawn behind the world and, with lightingFromSky, lights it (image-based lighting): its average colour from
/// each direction for the diffuse light, its reflection, blurred by roughness, for the specular.
struct Environment3D {
    glm::vec3 ambient{0.0F};                    ///< Linear RGB, multiplied by strength.
    float strength{1.0F};                       ///< How much of ambient reaches the surfaces.
    std::uint32_t shadowCascades{3};            ///< Directional lights: cascades of shadow maps, 1 to 4.
    std::uint32_t shadowResolution{1024};       ///< Texels of a side of one shadow map: 512, 1024 or 2048.
    float shadowDistance{60.0F};                ///< Directional lights: how far from the camera they cast shadows.
    SkyKind sky{SkyKind::kNone};                ///< What is drawn behind the world.
    TextureId skyTexture;                       ///< kTexture: the equirectangular image.
    glm::vec3 skyTop{0.25F, 0.45F, 0.85F};      ///< kGradient: the colour straight up, linear.
    glm::vec3 skyHorizon{0.75F, 0.85F, 0.95F};  ///< kGradient: the colour at the horizon, linear.
    glm::vec3 skyBottom{0.3F, 0.28F, 0.25F};    ///< kGradient: the colour straight down, linear.
    float skyIntensity{1.0F};                   ///< Multiplies the sky, drawn and as light.
    float skyRotation{0.0F};                    ///< Turns the sky around the vertical axis, in radians.
    bool lightingFromSky{true};                 ///< Whether the sky lights the world (added to ambient).

    [[nodiscard]] bool operator==(const Environment3D& other) const = default;
};

}  // namespace aylo::contract::renderer
