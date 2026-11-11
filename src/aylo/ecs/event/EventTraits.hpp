/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EventTraits
*/

#pragma once

#include <string_view>

#include "aylo/ecs/event/EventInfo.hpp"
#include "aylo/ecs/type/TypeTraits.hpp"

namespace aylo::ecs {
namespace detail {
/// @brief Checks that `T` can be an event and gathers its description.
template <typename T, typename... Fields>
[[nodiscard]] constexpr TypeDescription<sizeof...(Fields)> describeEvent(std::string_view name, Fields... fields);
}  // namespace detail

/// @brief True for a type declared with AYLO_ECS_EVENT.
template <typename T>
concept Event = Described<T> && TypeTraits<T>::kKind == TypeKind::kEvent;

/// @brief Everything known at compile time about an event declared with AYLO_ECS_EVENT: what every declared type
/// has (TypeTraits: name, hashes, fields, hooks, copyability), and how to describe it as an event.
template <Event T>
struct EventTraits : TypeTraits<T> {
    /// @return An EventInfo with the type's description; the id, presence and layout hash are left for the registry
    /// to set.
    [[nodiscard]] static EventInfo makeInfo();
};
}  // namespace aylo::ecs

#include "EventTraits.tpp"

/// @brief Declares an event: its stable name, and optionally the fields to reflect (to send or read it from Luau),
/// written once next to the type.
///
/// @details Use it right after the type, in the same namespace, ending with a semicolon. Any type that can be moved
/// and destroyed without throwing can be an event, including move-only ones; an event with reflected fields must be
/// standard-layout. A struct without fields is an event that carries no data.
/// @code
/// struct Collision {
///     aylo::ecs::Entity a;
///     aylo::ecs::Entity b;
/// };
/// AYLO_ECS_EVENT(Collision, "aylo.Collision", a, b);
///
/// struct GameOver {};
/// AYLO_ECS_EVENT(GameOver, "game.GameOver");
/// @endcode
/// @param Type The type, as named in the current namespace.
/// @param Name Its stable name, a string literal such as "aylo.Collision".
/// @param ... The fields to reflect, if any.
#define AYLO_ECS_EVENT(Type, Name, ...)                                                                 \
    AYLO_ECS_DETAIL_DESCRIBE(::aylo::ecs::detail::describeEvent, Type, Name __VA_OPT__(, ) __VA_ARGS__) \
    static_assert(true, "AYLO_ECS_EVENT must be followed by a semicolon")
