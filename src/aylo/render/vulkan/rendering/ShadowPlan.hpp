/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShadowPlan
*/

#pragma once

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <span>
#include <vector>

#include "aylo/contract/renderer/drawing/Light3D.hpp"
#include "render/vulkan/Export.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief Texels of a side of the shadow atlas: the shadow maps are tiles of it.
inline constexpr std::uint32_t kShadowAtlasSize = 4096;

/// @brief One shadow map: a tile of the atlas and what the shaders need to read it. The C++ side of `AyloShadowTile`
/// (the shaders ABI's common.glsl), 128 bytes, std430. Change both together.
struct ShadowTile {
    glm::mat4 viewProjection{1.0F};  ///< World to the light's clip space (Vulkan's: Y down, depth 0 to 1).
    glm::vec4 rect{0.0F};            ///< xy: its corner in the atlas, zw: its size, both 0 to 1.
    glm::vec4 bias{0.0F};            ///< x: depth bias (steps), y: normal bias (world units), z: slope bias, w: unused.
    glm::vec4 depth{0.0F};  ///< x, y: near and far planes; z: 1 perspective, 0 orthographic; w: a cascade's far end.
    glm::vec4 size{0.0F};   ///< x: the world width it covers (orthographic) or tan of half its angle (perspective);
                            ///< y: the light's size in world units (PCSS); z: a texel, 0 to 1; w: unused.
};
static_assert(sizeof(ShadowTile) == 128, "ShadowTile must match AyloShadowTile of common.glsl (std430)");

/// @brief The 3D camera the directional lights' cascades are fitted to.
struct ShadowCamera {
    glm::mat4 view{1.0F};             ///< World to camera space.
    float verticalFov{1.0F};          ///< Perspective: its vertical angle, radians; 0: orthographic.
    float orthographicHeight{10.0F};  ///< Orthographic: the height it shows.
    float aspect{1.0F};               ///< Width over height.
    float nearPlane{0.1F};            ///< Its near plane.
    float farPlane{1000.0F};          ///< Its far plane.
};

/// @brief The shadow maps of a frame: the tiles, and per light its first tile.
struct ShadowPlan {
    std::vector<ShadowTile> tiles;  ///< In the atlas, in order.
    std::vector<std::int32_t>
        firstTile;              ///< Per light given: its first tile, -1 without (a directional light has one per
                                ///< cascade, a point light six, the faces of a cube: +X, -X, +Y, -Y, +Z, -Z).
    std::uint32_t cascades{0};  ///< Cascades of each directional light.
};

/// @return The shadow maps of @p lights (in the order the shaders read them) that cast shadows, in tiles of
/// @p environment's shadowResolution (512, 1024 or 2048) in the atlas, as many as fit: a light that does not fit
/// casts none. A directional light gets environment.shadowCascades cascades over the first shadowDistance of
/// @p camera, split between a logarithmic and a uniform scheme, each a sphere snapped to its texels so it does not
/// shimmer as the camera moves; a spot light a perspective map of its cone; an area light one of a wide cone (120°)
/// along its facing; a point light six of 90°.
[[nodiscard]] AYLO_RENDER_VULKAN_API ShadowPlan planShadows(std::span<const contract::renderer::Light3D> lights,
                                                            const contract::renderer::Environment3D& environment,
                                                            const ShadowCamera& camera);

}  // namespace aylo::render::vulkan::rendering
