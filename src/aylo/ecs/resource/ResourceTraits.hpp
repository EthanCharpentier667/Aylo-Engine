/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ResourceTraits
*/

#pragma once

#include <string_view>

#include "aylo/ecs/resource/ResourceInfo.hpp"
#include "aylo/ecs/type/TypeTraits.hpp"

namespace aylo::ecs {
namespace detail {
/// @brief Checks that `T` can be a resource and gathers its description.
template <typename T, typename... Fields>
[[nodiscard]] constexpr TypeDescription<sizeof...(Fields)> describeResource(std::string_view name, Fields... fields);
}  // namespace detail

/// @brief True for a type declared with AYLO_ECS_RESOURCE.
template <typename T>
concept Resource = Described<T> && TypeTraits<T>::kKind == TypeKind::kResource;

/// @brief Everything known at compile time about a resource declared with AYLO_ECS_RESOURCE: what every declared
/// type has (TypeTraits: name, hashes, fields, hooks, copyability), and how to describe it as a resource.
template <Resource T>
struct ResourceTraits : TypeTraits<T> {
    /// @return A ResourceInfo with the type's description; the main-thread flag, id, presence, replication and
    /// layout hash are left for the registry to set.
    [[nodiscard]] static ResourceInfo makeInfo();
};
}  // namespace aylo::ecs

#include "ResourceTraits.tpp"

/// @brief Declares a resource: its stable name, and optionally the fields to reflect (to replicate it or reach it
/// from Luau), written once next to the type.
///
/// @details Use it right after the type, in the same namespace, ending with a semicolon. Any type that can be moved
/// and destroyed without throwing can be a resource, including classes with constructors, private members and
/// move-only types; a resource with reflected fields must be standard-layout.
/// @code
/// struct Time {
///     float delta;
///     float fixedDelta;
/// };
/// AYLO_ECS_RESOURCE(Time, "aylo.Time");
///
/// struct Score {
///     std::int32_t value;
/// };
/// AYLO_ECS_RESOURCE(Score, "game.Score", value);
/// @endcode
/// @param Type The type, as named in the current namespace.
/// @param Name Its stable name, a string literal such as "aylo.Time".
/// @param ... The fields to reflect, if any.
#define AYLO_ECS_RESOURCE(Type, Name, ...)                                                                 \
    AYLO_ECS_DETAIL_DESCRIBE(::aylo::ecs::detail::describeResource, Type, Name __VA_OPT__(, ) __VA_ARGS__) \
    static_assert(true, "AYLO_ECS_RESOURCE must be followed by a semicolon")
