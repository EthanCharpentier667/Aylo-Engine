/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioReflection
*/

#pragma once

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
static_assert(sizeof(aylo::contract::audiodevice::Bus) == 1, "Bus must match FieldType::kU8");
