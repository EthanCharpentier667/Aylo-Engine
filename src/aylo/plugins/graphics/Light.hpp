/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Light
*/

#pragma once

#include <array>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <string_view>
#include <type_traits>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"
#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::graphics {

/// @brief The shape of a Light3D.
enum class LightKind : std::uint8_t {
    kDirectional,  ///< The sun: from infinitely far, along the entity's -Z; lights everything the same.
    kPoint,        ///< A bulb: from the entity's position, all around.
    kSpot,         ///< A torch: from the entity's position, in a cone along its -Z.
    kArea,         ///< A window, a screen: from a rectangle centred on the entity, facing its -Z, X its width.
};

}  // namespace aylo::plugins::graphics

/// @brief A LightKind field is reflected as kU8, with the names scripts write.
template <>
struct aylo::ecs::FieldTypeOf<aylo::plugins::graphics::LightKind>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};

/// @brief The names scripts and capsule files give the values of a LightKind field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::plugins::graphics::LightKind> {
    static constexpr std::array<std::string_view, 4> kNames{"directional", "point", "spot", "area"};
};

namespace aylo::plugins::graphics {

/// @brief The component ("aylo.Light3D") that makes a 3D entity a light: the graphics plugin hands every enabled one to
/// the renderer each frame (IRenderer::setLights3D()), placed and turned by its GlobalTransform3D (it shines along -Z).
/// @details Physical units, as glTF and Blender: `power` in watts for point, spot and area lights, `intensity` in lux
/// for a directional one, so a scene exported from a 3D tool keeps its balance. Lit materials (R3D-06) read them; an
/// unlit model ignores them.
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform3D{.position = {0.0F, 3.0F, 0.0F}})
///     .add(aylo::plugins::graphics::Light3D{.kind = aylo::plugins::graphics::LightKind::kPoint, .power = 800.0F});
/// @endcode
struct Light3D {
    LightKind kind = LightKind::kPoint;  ///< Its shape.
    glm::vec3 color{1.0F};               ///< Linear RGB.
    float power = 1000.0F;               ///< Point, spot and area: watts.
    float intensity = 1.0F;              ///< Directional: lux (W/m²).
    float range = 0.0F;                  ///< Distance past which it lights nothing; 0: no limit.
    float innerConeAngle = 0.0F;         ///< Spot: the half-angle lit fully, in degrees.
    float outerConeAngle = 45.0F;        ///< Spot: the half-angle past which nothing is lit, in degrees.
    glm::vec2 size{1.0F};                ///< Area: its width (along X) and height (along Y), in world units.
    bool castShadows = false;            ///< Whether it casts shadows.
    bool enabled = true;                 ///< Off: it lights nothing.
    float shadowSoftness = 1.0F;         ///< 0: hard; 1: as soft as a light of its size; more: softer.
    float shadowBias = 1.0F;             ///< Raise it if a lit surface shows dark stripes (acne).
    float shadowNormalBias = 1.0F;       ///< The same, along the surface's normal.
};
AYLO_ECS_COMPONENT(Light3D, "aylo.Light3D", kind, color, power, intensity, range, innerConeAngle, outerConeAngle, size,
                   castShadows, enabled, shadowSoftness, shadowBias, shadowNormalBias);

/// @brief The component ("aylo.Environment3D") that gives the 3D world its ambient light: the light that reaches every
/// surface, wherever it is, so what no Light3D reaches is not black. The one with the highest priority is used; without
/// one, there is no ambient light.
struct Environment3D {
    glm::vec3 ambient{0.0F};                ///< Linear RGB.
    float strength = 1.0F;                  ///< Multiplies ambient.
    std::int32_t priority = 0;              ///< Among several, the highest one is used.
    std::uint32_t shadowCascades = 3;       ///< Directional lights: cascades of shadow maps, 1 to 4.
    std::uint32_t shadowResolution = 1024;  ///< Texels of a side of a shadow map: 512, 1024 or 2048.
    float shadowDistance = 60.0F;           ///< Directional lights: how far from the camera they cast shadows.
};
AYLO_ECS_COMPONENT(Environment3D, "aylo.Environment3D", ambient, strength, priority, shadowCascades, shadowResolution,
                   shadowDistance);

}  // namespace aylo::plugins::graphics
