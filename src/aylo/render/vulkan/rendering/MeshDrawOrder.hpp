/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshDrawOrder
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <span>
#include <vector>

#include "render/vulkan/Export.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief Where a picture of the 3D world is seen from, to sort its blended draws.
struct MeshViewpoint {
    glm::vec3 position{0.0F};              ///< The camera, in the world.
    glm::vec3 forward{0.0F, 0.0F, -1.0F};  ///< Where it looks, unit.
    bool perspective{true};                ///< Farther is by distance (perspective) or along forward.
};

/// @brief One draw to order: where it is and whether it is blended.
struct MeshDrawKey {
    glm::vec3 center{0.0F};   ///< The centre of its box, in the world.
    bool transparent{false};  ///< Blended: drawn after the opaque ones, back to front.
};

/// @brief Fills @p order with the indices of @p draws in the order they are drawn: the opaque ones first, in their
/// order, then the blended ones from the farthest from @p viewpoint to the nearest (by distance for a perspective, by
/// depth along its forward for an orthographic camera); equal ones keep their order. @p order is reused, so a frame
/// allocates nothing once it is big enough.
AYLO_RENDER_VULKAN_API void orderMeshDraws(std::span<const MeshDrawKey> draws, const MeshViewpoint& viewpoint,
                                           std::vector<std::uint32_t>& order);

}  // namespace aylo::render::vulkan::rendering
