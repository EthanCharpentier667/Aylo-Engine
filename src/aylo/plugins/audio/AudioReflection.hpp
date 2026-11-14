/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioReflection
*/

#pragma once

#include <array>
#include <string_view>
#include <type_traits>

#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"

// The ECS side of the audio contract, which knows no ECS. The ECS reflects scalars only: a Bus field is reflected as
// its underlying byte. The specialization must sit outside the contract's namespace, in one that encloses aylo::ecs.
/// @brief A Bus field is reflected as kU8.
template <>
struct aylo::ecs::FieldTypeOf<aylo::contract::audiodevice::Bus>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kU8> {};
/// @brief The names scripts and capsule files give the values of a Bus field.
template <>
struct aylo::ecs::FieldEnumNames<aylo::contract::audiodevice::Bus> {
    static constexpr std::array<std::string_view, 2> kNames{"music", "effects"};
};
static_assert(sizeof(aylo::contract::audiodevice::Bus) == 1, "Bus must match FieldType::kU8");
