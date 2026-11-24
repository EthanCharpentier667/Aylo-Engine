/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StaticMeshDrawData
*/

#pragma once

#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "render/vulkan/Export.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief One static mesh draw as the shaders read it: the C++ side of `AyloMeshDraw` (the shaders ABI's common.glsl,
/// AYLO_MESH), an element of the frame's storage buffer of mesh draws (std430), 304 bytes. Change both together.
struct StaticMeshDrawData {
    glm::mat4 model{1.0F};         ///< The model's space to the world's. Offset 0.
    glm::mat4 inverseModel{1.0F};  ///< The world's space to the model's. Offset 64.
    glm::mat4 normal{1.0F};        ///< The inverse transpose of model: normals to the world. Offset 128.
    glm::vec4 color{1.0F};         ///< The draw's colour, straight alpha. Offset 192.
    glm::vec4 boundsMin{0.0F};     ///< xyz: the smallest corner of the model's box. Offset 208.
    glm::vec4 boundsMax{0.0F};     ///< xyz: the largest corner. Offset 224.
    contract::renderer::ShaderInstanceValues instance{};  ///< The `instance uniform` values. Offset 240.
};

static_assert(offsetof(StaticMeshDrawData, inverseModel) == 64 && offsetof(StaticMeshDrawData, normal) == 128 &&
                  offsetof(StaticMeshDrawData, color) == 192 && offsetof(StaticMeshDrawData, boundsMin) == 208 &&
                  offsetof(StaticMeshDrawData, boundsMax) == 224 && offsetof(StaticMeshDrawData, instance) == 240 &&
                  sizeof(StaticMeshDrawData) == 304,
              "StaticMeshDrawData must match AyloMeshDraw of common.glsl (std430)");

/// @return What the shaders read for a draw of a mesh of @p bounds at @p world, with @p color and @p instance: the
/// matrices the model matrix gives (its inverse; the normal matrix, which keeps normals perpendicular to a surface
/// scaled unevenly). A matrix that cannot be inverted (a scale of 0) gets identity inverses, so nothing reads NaN.
/// boundsMin.w: whether shadows fall on it.
[[nodiscard]] AYLO_RENDER_VULKAN_API StaticMeshDrawData makeStaticMeshDrawData(
    const glm::mat4& world, const glm::vec4& color, const contract::renderer::StaticMeshBounds& bounds,
    const contract::renderer::ShaderInstanceValues& instance, bool receiveShadows = true) noexcept;

}  // namespace aylo::render::vulkan::rendering
