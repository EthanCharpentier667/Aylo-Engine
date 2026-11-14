/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShapeRenderer
*/

#pragma once

#include <array>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <string_view>
#include <type_traits>

#include "aylo/contract/renderer/drawing/BlendMode.hpp"
#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Shape.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"
#include "plugins/GlmReflection.hpp"       // IWYU pragma: keep
#include "plugins/GraphicsReflection.hpp"  // IWYU pragma: keep

// The ECS reflects scalars only: a ShapeKind field is reflected as its underlying byte. The specialization must sit
// outside the plugin's namespace, in one that encloses aylo::ecs, and here rather than in Shape.hpp, which the
// renderers include without the ECS. BlendMode's is in plugins/GraphicsReflection.hpp.
/// @brief A contract::renderer::ShapeKind field is reflected as kU8.
template <>
struct aylo::ecs::FieldTypeOf<aylo::contract::renderer::ShapeKind>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a ShapeKind field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::contract::renderer::ShapeKind> {
    static constexpr std::array<std::string_view, 4> kNames{"rect", "circle", "capsule", "line"};
};
static_assert(sizeof(aylo::contract::renderer::ShapeKind) == 1, "ShapeKind must match FieldType::kU8");

namespace aylo::plugins::shape {
/// @brief Draws an entity as a plain shape, filled and/or outlined: the component a render system turns into a
/// contract::renderer::Shape (walls, panels, bars, markers).
///
/// @details Only what to draw: where comes from the entity's GlobalTransform2D. Rectangles, circles and capsules are
/// centred on its position and turn with its rotation; a line runs from its position to `end`, which turns and scales
/// with it. The sizes are in world units, times the entity's scale: `size` on both axes, the radii, the corner radius
/// and the outline on X. The ShapePlugin's Render system draws the shapes after the sprites, as IRenderer::draw()
/// draws every shape over them, sorted by layer among themselves (higher on top). Its fields are reflected (Luau,
/// network): `kind` as its byte (0 rect, 1 circle, 2 capsule, 3 line).
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform2D{.position = {400.0F, 300.0F}})
///     .add(aylo::plugins::shape::ShapeRenderer{.kind = aylo::contract::renderer::ShapeKind::kCircle,
///                                                 .radius = 36.0F,
///                                                 .fillColor = {0.25F, 0.27F, 0.32F, 1.0F},
///                                                 .outlineColor = {0.55F, 0.6F, 0.7F, 1.0F},
///                                                 .outlineThickness = 2.0F});
/// @endcode
struct ShapeRenderer {
    contract::renderer::ShapeKind kind =
        contract::renderer::ShapeKind::kRect;  ///< Which fields below describe the shape.
    glm::vec2 size{0.0F};                      ///< kRect: full width and height.
    glm::vec2 end{0.0F};                       ///< kLine: the end, from the entity's position, in its local space.
    float radius = 0.0F;                       ///< kCircle, kCapsule: the radius; kLine: half the thickness.
    float halfHeight = 0.0F;    ///< kCapsule: half the length of the straight part, along its local Y axis.
    float cornerRadius = 0.0F;  ///< kRect: the radius of the rounded corners; 0: sharp.
    contract::renderer::Color fillColor{1.0F};     ///< Fully transparent: outline only.
    contract::renderer::Color outlineColor{0.0F};  ///< The outline's colour.
    float outlineThickness = 0.0F;                 ///< Drawn inside the shape. 0: no outline.
    std::int32_t layer{0};                         ///< Draw order among the shapes: higher layers are drawn on top.
    contract::renderer::BlendMode blend{contract::renderer::BlendMode::kAlpha};  ///< kAlpha or kAdditive.
};
AYLO_ECS_COMPONENT(ShapeRenderer, "aylo.ShapeRenderer", kind, size, end, radius, halfHeight, cornerRadius, fillColor,
                   outlineColor, outlineThickness, layer, blend);
}  // namespace aylo::plugins::shape
