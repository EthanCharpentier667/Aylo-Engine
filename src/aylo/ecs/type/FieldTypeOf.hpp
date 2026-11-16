/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FieldTypeOf
*/

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldList.hpp"

namespace aylo::ecs {
/// @brief Maps a C++ type to the FieldType it is reflected as, in `value`. Empty for unsupported types.
///
/// @details The ECS maps the scalar types itself. It is also the extension point for other types: specialize it,
/// at global scope or in `aylo::ecs`, to reflect a math type of your own. The type must have the size and
/// alignment of the FieldType (see getFieldLayout()).
/// @code
/// template <>
/// struct aylo::ecs::FieldTypeOf<glm::vec2>
///     : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kVec2> {};
/// @endcode
template <typename T>
struct FieldTypeOf {};

template <>
struct FieldTypeOf<bool> : std::integral_constant<FieldType, FieldType::kBool> {};
template <>
struct FieldTypeOf<std::int8_t> : std::integral_constant<FieldType, FieldType::kI8> {};
template <>
struct FieldTypeOf<std::int16_t> : std::integral_constant<FieldType, FieldType::kI16> {};
template <>
struct FieldTypeOf<std::int32_t> : std::integral_constant<FieldType, FieldType::kI32> {};
template <>
struct FieldTypeOf<std::int64_t> : std::integral_constant<FieldType, FieldType::kI64> {};
template <>
struct FieldTypeOf<std::uint8_t> : std::integral_constant<FieldType, FieldType::kU8> {};
template <>
struct FieldTypeOf<std::uint16_t> : std::integral_constant<FieldType, FieldType::kU16> {};
template <>
struct FieldTypeOf<std::uint32_t> : std::integral_constant<FieldType, FieldType::kU32> {};
template <>
struct FieldTypeOf<std::uint64_t> : std::integral_constant<FieldType, FieldType::kU64> {};
template <>
struct FieldTypeOf<float> : std::integral_constant<FieldType, FieldType::kF32> {};
template <>
struct FieldTypeOf<double> : std::integral_constant<FieldType, FieldType::kF64> {};

/// @brief A reference to another entity: cleared when that entity is despawned, remapped by the network.
template <>
struct FieldTypeOf<Entity> : std::integral_constant<FieldType, FieldType::kEntity> {};

/// @brief The bytes of a `std::string` field.
template <>
struct FieldTypeOf<detail::TextByte> : std::integral_constant<FieldType, FieldType::kText> {};

/// @brief True when a FieldTypeOf specialization reflects `T`.
template <typename T>
concept HasFieldType = requires {
    { FieldTypeOf<T>::value } -> std::convertible_to<FieldType>;
};

/// @brief The names of the values of an enum reflected as an integer, in `kNames`: value `i` is `kNames[i]`, so the
/// enum's values must run from 0 without gaps. Empty for other types.
///
/// @details Specialize it next to the enum's FieldTypeOf, with the names scripts and capsule files write: short,
/// lowercase, as the manual spells them. A field of the enum then reads as its name in a script and takes the name or
/// the number.
/// @code
/// template <>
/// struct aylo::ecs::FieldEnumNames<aylo::plugins::physics::ShapeType> {
///     static constexpr std::array<std::string_view, 3> kNames{"box", "circle", "capsule"};
/// };
/// @endcode
template <typename T>
struct FieldEnumNames {};

/// @brief True when a FieldEnumNames specialization names the values of `T`.
template <typename T>
concept HasEnumNames = requires {
    { std::span<const std::string_view>{FieldEnumNames<T>::kNames} };
};

/// @brief The units of a type stored as a `vec2` of a number and the index of its unit, in `kUnits`: unit `i` is
/// `kUnits[i]`, and a plain number is in unit `kBareNumber`. Empty for other types.
///
/// @details Specialize it next to the type's FieldTypeOf (which must be FieldType::kVec2): a field of the type then
/// reads in a script as a number (in the bare unit), `"<number><suffix>"` or a keyword, and takes the same, or the
/// `{ x = value, y = unit }` table it is stored as. Serialization and replication keep the two floats.
/// @code
/// template <>
/// struct aylo::ecs::FieldUnits<aylo::plugins::ui::UiLength> {
///     static constexpr std::array<aylo::ecs::FieldUnit, 3> kUnits{{{"auto", true}, {"px"}, {"%"}}};
///     static constexpr std::uint8_t kBareNumber = 1;
/// };
/// @endcode
template <typename T>
struct FieldUnits {};

/// @brief True when a FieldUnits specialization names the units of `T`.
template <typename T>
concept HasFieldUnits = requires {
    { std::span<const FieldUnit>{FieldUnits<T>::kUnits} };
    { FieldUnits<T>::kBareNumber } -> std::convertible_to<std::uint8_t>;
};

namespace detail {
/// @brief A `std::array` field: `std::array<float, 4>` is 4 values of `float`. C arrays are not supported.
template <typename T, std::size_t N>
struct FieldElement<std::array<T, N>> {
    using Type = T;
    static constexpr auto kCount = static_cast<std::uint32_t>(N);
    static constexpr const ListAccess* kList = nullptr;
};
}  // namespace detail
}  // namespace aylo::ecs
