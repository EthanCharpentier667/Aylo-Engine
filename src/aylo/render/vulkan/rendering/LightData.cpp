/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** LightData
*/

#include "LightData.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint4.hpp>
#include <glm/geometric.hpp>
#include <span>
#include <vector>

#include "aylo/contract/renderer/drawing/Light3D.hpp"

namespace aylo::render::vulkan::rendering {

namespace {

using contract::renderer::Light3D;
using contract::renderer::LightKind3D;

/// @return @p light as the shaders read it.
GpuLight toGpu(const Light3D& light) noexcept {
    const float inner = std::clamp(light.innerCone, 0.0F, light.outerCone);
    return GpuLight{.position = glm::vec4{light.position, std::max(light.range, 0.0F)},
                    .direction = glm::vec4{glm::normalize(light.direction), static_cast<float>(light.kind)},
                    .color = glm::vec4{light.color, light.intensity},
                    .cone = glm::vec4{std::cos(inner), std::cos(light.outerCone), light.size * 0.5F},
                    .right = glm::vec4{glm::normalize(light.right), light.castShadows ? 1.0F : 0.0F}};
}

/// @return Whether @p light lights anything.
bool lights(const Light3D& light) noexcept {
    return light.intensity > 0.0F && glm::dot(light.color, light.color) > 0.0F &&
           glm::dot(light.direction, light.direction) > 0.0F;
}

}  // namespace

void packLights(std::span<const Light3D> lights3D, const contract::renderer::Environment3D& environment,
                const glm::vec3& camera, std::vector<std::byte>& bytes) {
    std::vector<const Light3D*> kept;
    for (const Light3D& light : lights3D) {
        if (lights(light)) {
            kept.push_back(&light);
        }
    }
    if (kept.size() > contract::renderer::kMaxLights3D) {
        // The sun lights everything: directional lights first, then the nearest others.
        const auto farness = [&camera](const Light3D* light) {
            const glm::vec3 offset = light->position - camera;
            return light->kind == LightKind3D::kDirectional ? -1.0F : glm::dot(offset, offset);
        };
        std::ranges::stable_sort(kept, {}, farness);
        kept.resize(contract::renderer::kMaxLights3D);
    }
    const GpuLightsHeader header{.ambient = glm::vec4{environment.ambient * environment.strength, 0.0F},
                                 .count = glm::uvec4{static_cast<std::uint32_t>(kept.size()), 0U, 0U, 0U}};
    bytes.resize(sizeof(GpuLightsHeader) + (kept.size() * sizeof(GpuLight)));
    std::memcpy(bytes.data(), &header, sizeof(header));
    std::size_t offset = sizeof(GpuLightsHeader);
    for (const Light3D* light : kept) {
        const GpuLight gpu = toGpu(*light);
        std::memcpy(std::span{bytes}.subspan(offset, sizeof(gpu)).data(), &gpu, sizeof(gpu));
        offset += sizeof(gpu);
    }
}

}  // namespace aylo::render::vulkan::rendering
