/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GraphicsReflection
*/

#pragma once

#include <array>
#include <string_view>
#include <type_traits>

#include "aylo/contract/renderer/drawing/BlendMode.hpp"
#include "aylo/contract/renderer/math/Rect.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"

// Lets the components reflect the renderer contract's graphics types (SpriteRenderer, ShapeRenderer): the ECS maps
// scalars only, so an enum is reflected as its underlying integer and a Rect as the four floats it holds. The
// specializations sit outside the plugins' namespaces, in one that encloses aylo::ecs, and here rather than next to
// the types, which the renderers include without the ECS.

/// @brief A contract::renderer::BlendMode field is reflected as kU8.
template <>
struct aylo::ecs::FieldTypeOf<aylo::contract::renderer::BlendMode>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a BlendMode field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::contract::renderer::BlendMode> {
    static constexpr std::array<std::string_view, 3> kNames{"alpha", "additive", "cutout"};
};
static_assert(sizeof(aylo::contract::renderer::BlendMode) == 1, "BlendMode must match FieldType::kU8");

/// @brief A contract::renderer::math::Rect field is reflected as kVec4: x and y are its position, z and w its width and
/// height.
template <>
struct aylo::ecs::FieldTypeOf<aylo::contract::renderer::math::Rect>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kVec4> {};
static_assert(sizeof(aylo::contract::renderer::math::Rect) == 16 && alignof(aylo::contract::renderer::math::Rect) == 4,
              "Rect must match FieldType::kVec4");
static_assert(std::is_standard_layout_v<aylo::contract::renderer::math::Rect> &&
                  std::is_trivially_copyable_v<aylo::contract::renderer::math::Rect>,
              "Rect must be four floats in a row: x, y, width, height");
