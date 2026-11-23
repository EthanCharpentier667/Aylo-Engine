/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StaticMeshConstants
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float4.hpp>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "render/vulkan/Export.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief The push constants of a static mesh part: what the shaders ABI reads as `ayloPart`, 80 bytes. The rest of
/// the draw is in the frame's buffer of mesh draws, at index `draw`.
struct StaticMeshConstants {
    glm::vec4 baseColor{1.0F};                      ///< The part's base colour.
    glm::vec4 emissive{0.0F, 0.0F, 0.0F, 1.0F};     ///< rgb: emissive; w: the normal map's scale.
    glm::vec4 surface{0.0F, 0.5F, 1.0F, 0.5F};      ///< Metallic, roughness, occlusion strength, alpha cutoff.
    glm::vec4 uvTransform{0.0F, 0.0F, 1.0F, 1.0F};  ///< Offset (xy) and scale (zw) of the coordinates.
    std::uint32_t draw{0};                          ///< The draw's index in the buffer of mesh draws.
    std::uint32_t flags{0};                         ///< The maps it has, and its alpha mode (bits 4 and 5).
    float uvRotation{0.0F};                         ///< The rotation of the coordinates, radians.
    std::uint32_t padding{0};                       ///< To a multiple of 16 bytes.

    /// @return The constants of @p part, drawn as draw @p draw.
    [[nodiscard]] AYLO_RENDER_VULKAN_API static StaticMeshConstants of(const contract::renderer::StaticMeshPart& part,
                                                                       std::uint32_t draw) noexcept;
};

}  // namespace aylo::render::vulkan::rendering
