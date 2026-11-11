/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CallTraits
*/

#pragma once

#ifndef AYLO_ECS_CALL_TRAITS_INCLUDING_TPP
#error "CallTraits.tpp is an implementation detail of CallTraits.hpp: include CallTraits.hpp instead"
#endif

#include <string_view>
#include <type_traits>

#include "aylo/ecs/type/TypeTraits.hpp"

namespace aylo::ecs::detail {
template <typename T, typename... Fields>
constexpr TypeDescription<sizeof...(Fields)> describeCallData(std::string_view name, Fields... fields) {
    static_assert(std::is_object_v<T> && !std::is_array_v<T> && !std::is_const_v<T> && !std::is_volatile_v<T>,
                  "AYLO_ECS_CALL_DATA: call data must be a plain, non-const object type.");
    static_assert(std::is_standard_layout_v<T>,
                  "AYLO_ECS_CALL_DATA: call data must be standard-layout (all members public, no reference members, "
                  "no virtual base).");
    static_assert(std::is_nothrow_move_constructible_v<T>, "AYLO_ECS_CALL_DATA: moving call data must not throw.");
    static_assert(std::is_nothrow_destructible_v<T>, "AYLO_ECS_CALL_DATA: destroying call data must not throw.");
    static_assert(std::is_default_constructible_v<T>,
                  "AYLO_ECS_CALL_DATA: call data must be default-constructible: a script leaves out the fields that "
                  "keep their default.");
    return TypeDescription<sizeof...(Fields)>{.name = name, .kind = TypeKind::kCallData, .fields = {fields...}};
}
}  // namespace aylo::ecs::detail
