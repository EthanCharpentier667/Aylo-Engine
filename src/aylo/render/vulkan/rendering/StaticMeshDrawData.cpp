/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StaticMeshDrawData
*/

#include "StaticMeshDrawData.hpp"

#include <cmath>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/matrix.hpp>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"

namespace aylo::render::vulkan::rendering {

namespace {

/// @brief Determinants below this are treated as 0: the matrix flattens the model and has no useful inverse.
constexpr float kMinDeterminant = 1e-12F;

}  // namespace

StaticMeshDrawData makeStaticMeshDrawData(const glm::mat4& world, const glm::vec4& color,
                                          const contract::renderer::StaticMeshBounds& bounds,
                                          const contract::renderer::ShaderInstanceValues& instance,
                                          bool receiveShadows) noexcept {
    StaticMeshDrawData data{.model = world,
                            .color = color,
                            .boundsMin = glm::vec4{bounds.min, receiveShadows ? 1.0F : 0.0F},
                            .boundsMax = glm::vec4{bounds.max, 0.0F},
                            .instance = instance};
    if (std::abs(glm::determinant(world)) > kMinDeterminant) {
        data.inverseModel = glm::inverse(world);
        data.normal = glm::transpose(data.inverseModel);
    }
    return data;
}

}  // namespace aylo::render::vulkan::rendering
