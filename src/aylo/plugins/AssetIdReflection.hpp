/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AssetIdReflection
*/

#pragma once

#include <type_traits>

#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"
#include "core/asset/AssetId.hpp"

/// @brief An AssetId field is reflected as kAssetId: written as is by the network, the same in every process.
template <>
struct aylo::ecs::FieldTypeOf<aylo::core::asset::AssetId>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kAssetId> {};
