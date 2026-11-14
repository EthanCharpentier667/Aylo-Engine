/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GlmReflection
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <type_traits>

#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"

// Lets the plugins' components reflect glm vector fields (AYLO_ECS_COMPONENT(Transform2D, ..., position, ...)): the ECS
// maps scalars only and depends on no math library, so the plugins, which use glm, tell it how. Each vector has the
// size and alignment of its FieldType: plain floats, no SIMD padding (GLM_FORCE_XYZW_ONLY, no aligned gentypes).

/// @brief A glm::vec2 field is reflected as kVec2.
template <>
struct aylo::ecs::FieldTypeOf<glm::vec2> : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kVec2> {};
/// @brief A glm::vec3 field is reflected as kVec3.
template <>
struct aylo::ecs::FieldTypeOf<glm::vec3> : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kVec3> {};
/// @brief A glm::vec4 field is reflected as kVec4.
template <>
struct aylo::ecs::FieldTypeOf<glm::vec4> : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kVec4> {};

static_assert(sizeof(glm::vec2) == 8 && alignof(glm::vec2) == 4, "glm::vec2 must match FieldType::kVec2");
static_assert(sizeof(glm::vec3) == 12 && alignof(glm::vec3) == 4, "glm::vec3 must match FieldType::kVec3");
static_assert(sizeof(glm::vec4) == 16 && alignof(glm::vec4) == 4, "glm::vec4 must match FieldType::kVec4");
