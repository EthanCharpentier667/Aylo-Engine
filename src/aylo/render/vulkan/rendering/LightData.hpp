/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** LightData
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint4.hpp>
#include <span>
#include <vector>

#include "aylo/contract/renderer/drawing/Light3D.hpp"
#include "render/vulkan/Export.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief One light as the shaders read it: the C++ side of `AyloLight` (the shaders ABI's common.glsl, AYLO_MESH), an
/// element of the frame's storage buffer of lights (std430), 80 bytes. Change both together. Laid out as a clustered
/// pass would read it, so one can come later without changing the shaders.
struct GpuLight {
    glm::vec4 position{0.0F};   ///< xyz: where it is; w: its range, 0 for none.
    glm::vec4 direction{0.0F};  ///< xyz: where it shines, unit; w: its kind (contract::renderer::LightKind3D).
    glm::vec4 color{0.0F};      ///< rgb: linear colour; w: its intensity, in watts or lux.
    glm::vec4 cone{0.0F};       ///< x, y: cos of the spot's inner and outer half-angles; z, w: half the area's size.
    glm::vec4 right{0.0F};      ///< xyz: the area's width axis, unit; w: its first shadow map + 1, 0 without.
};

/// @brief The start of the buffer of lights: the C++ side of the head of `AyloLights`, 32 bytes, the lights after it.
struct GpuLightsHeader {
    glm::vec4 ambient{0.0F};  ///< rgb: the ambient light, its strength applied; w: unused.
    glm::uvec4 count{0U};     ///< x: how many lights follow; y: the cascades of a directional light.
};

static_assert(sizeof(GpuLight) == 80 && offsetof(GpuLight, direction) == 16 && offsetof(GpuLight, right) == 64,
              "GpuLight must match AyloLight of common.glsl (std430)");
static_assert(sizeof(GpuLightsHeader) == 32, "GpuLightsHeader must match the head of AyloLights (std430)");

/// @return The lights of @p lights a frame draws with, in the order the shaders read them: at most
/// contract::renderer::kMaxLights3D, the directional lights then the others nearest to @p camera past the cap; a light
/// with no intensity or a black colour left out.
[[nodiscard]] AYLO_RENDER_VULKAN_API std::vector<contract::renderer::Light3D> selectLights(
    std::span<const contract::renderer::Light3D> lights, const glm::vec3& camera);

/// @brief Writes @p selected (selectLights()) and @p environment as the buffer of lights reads them into @p bytes
/// (cleared first): the header, then the lights. @p firstShadowTiles gives each light its first shadow map, -1 for
/// none (empty: none), and @p cascades the cascades of a directional light.
AYLO_RENDER_VULKAN_API void packLights(std::span<const contract::renderer::Light3D> selected,
                                       const contract::renderer::Environment3D& environment,
                                       std::span<const std::int32_t> firstShadowTiles, std::uint32_t cascades,
                                       std::vector<std::byte>& bytes);

}  // namespace aylo::render::vulkan::rendering
