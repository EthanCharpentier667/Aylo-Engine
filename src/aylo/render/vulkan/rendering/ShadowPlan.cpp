/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShadowPlan
*/

#include "ShadowPlan.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <span>
#include <vector>

#include "aylo/contract/renderer/drawing/Light3D.hpp"

namespace aylo::render::vulkan::rendering {

namespace {

using contract::renderer::Light3D;
using contract::renderer::LightKind3D;

constexpr float kShadowNear = 0.05F;          ///< Near plane of spot and point maps.
constexpr float kDefaultShadowRange = 50.0F;  ///< Far plane of a spot or point map without a range.
constexpr float kSunDiameter = 0.0093F;       ///< The sun's angular diameter, radians: a shadowSoftness of 1.
constexpr float kBulbSize = 0.1F;             ///< A point or spot light's size at a shadowSoftness of 1, metres.
constexpr float kAreaShadowAngle = 2.0944F;   ///< The cone an area light's map covers, radians (120°).
constexpr float kCascadeSplitBlend = 0.7F;    ///< Logarithmic share of the practical split scheme.
constexpr float kCasterReach = 200.0F;        ///< How far toward the sun a cascade looks for casters.

/// @return @p projection with Y down, as the 3D camera's: the picture is not mirrored, so the faces keep their winding.
glm::mat4 flipY(glm::mat4 projection) {
    projection[1][1] *= -1.0F;  // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    return projection;
}

/// @return A unit vector across @p direction, to look along it.
glm::vec3 upFor(const glm::vec3& direction) {
    return std::abs(direction.y) > 0.99F ? glm::vec3{0.0F, 0.0F, 1.0F} : glm::vec3{0.0F, 1.0F, 0.0F};
}

/// @return The corners, in the world, of the slice of @p camera's frustum between @p near and @p far.
std::array<glm::vec3, 8> sliceCorners(const ShadowCamera& camera, float near, float far) {
    const glm::mat4 cameraToWorld = glm::inverse(camera.view);
    std::array<glm::vec3, 8> corners{};
    std::size_t index = 0;
    for (const float distance : {near, far}) {
        const float halfHeight = camera.verticalFov > 0.0F ? distance * std::tan(camera.verticalFov * 0.5F)
                                                           : camera.orthographicHeight * 0.5F;
        const float halfWidth = halfHeight * camera.aspect;
        for (const float x : {-halfWidth, halfWidth}) {
            for (const float y : {-halfHeight, halfHeight}) {
                corners.at(index++) = glm::vec3{cameraToWorld * glm::vec4{x, y, -distance, 1.0F}};
            }
        }
    }
    return corners;
}

/// @brief Plans one cascade of a directional light shining along @p direction, between @p near and @p far of
/// @p camera, at @p resolution texels.
ShadowTile cascade(const Light3D& light, const ShadowCamera& camera, float near, float far, std::uint32_t resolution) {
    const std::array<glm::vec3, 8> corners = sliceCorners(camera, near, far);
    glm::vec3 center{0.0F};
    for (const glm::vec3& corner : corners) {
        center += corner / 8.0F;
    }
    float radius = 0.0F;
    for (const glm::vec3& corner : corners) {
        radius = std::max(radius, glm::distance(corner, center));
    }
    radius = std::ceil(radius * 16.0F) / 16.0F;  // a size that does not change with the camera's turns
    const glm::vec3 direction = glm::normalize(light.direction);
    const glm::mat4 lightView = glm::lookAtRH(glm::vec3{0.0F}, direction, upFor(direction));
    // Snapped to whole texels of the map, so a moving camera does not make the edges shimmer.
    const float texel = (2.0F * radius) / static_cast<float>(resolution);
    glm::vec3 snapped = glm::vec3{lightView * glm::vec4{center, 1.0F}};
    snapped.x = std::floor(snapped.x / texel) * texel;
    snapped.y = std::floor(snapped.y / texel) * texel;
    const glm::mat4 projection =
        flipY(glm::orthoRH_ZO(snapped.x - radius, snapped.x + radius, snapped.y - radius, snapped.y + radius,
                              -snapped.z - radius - kCasterReach, -snapped.z + radius));
    return ShadowTile{
        .viewProjection = projection * lightView,
        .depth = glm::vec4{0.0F, (2.0F * radius) + kCasterReach, 0.0F, far},
        .size = glm::vec4{2.0F * radius, std::tan(kSunDiameter * 0.5F) * light.shadowSoftness, 0.0F, 0.0F}};
}

/// @return A perspective map from @p position along @p direction, of @p angle radians, to @p far.
ShadowTile perspective(const glm::vec3& position, const glm::vec3& direction, float angle, float far, float size) {
    const glm::mat4 view = glm::lookAtRH(position, position + direction, upFor(direction));
    return ShadowTile{.viewProjection = flipY(glm::perspectiveRH_ZO(angle, 1.0F, kShadowNear, far)) * view,
                      .depth = glm::vec4{kShadowNear, far, 1.0F, 0.0F},
                      .size = glm::vec4{std::tan(angle * 0.5F), size, 0.0F, 0.0F}};
}

/// @return The tiles @p light needs, in order.
std::vector<ShadowTile> tilesOf(const Light3D& light, const contract::renderer::Environment3D& environment,
                                const ShadowCamera& camera, std::uint32_t resolution) {
    std::vector<ShadowTile> tiles;
    const float far = light.range > 0.0F ? light.range : kDefaultShadowRange;
    const glm::vec3 direction = glm::normalize(light.direction);
    switch (light.kind) {
        case LightKind3D::kDirectional: {
            const std::uint32_t cascades = std::clamp(environment.shadowCascades, 1U, 4U);
            const float start = std::max(camera.nearPlane, 0.01F);
            const float end = std::max(std::min(environment.shadowDistance, camera.farPlane), start * 2.0F);
            float near = start;
            for (std::uint32_t index = 1; index <= cascades; ++index) {
                const float share = static_cast<float>(index) / static_cast<float>(cascades);
                const float logarithmic = start * std::pow(end / start, share);
                const float uniform = start + ((end - start) * share);
                const float split = (kCascadeSplitBlend * logarithmic) + ((1.0F - kCascadeSplitBlend) * uniform);
                tiles.push_back(cascade(light, camera, near, split, resolution));
                near = split;
            }
            break;
        }
        case LightKind3D::kSpot:
            tiles.push_back(perspective(light.position, direction, std::min(2.0F * light.outerCone, 3.05F) + 0.05F, far,
                                        kBulbSize * light.shadowSoftness));
            break;
        case LightKind3D::kArea:
            tiles.push_back(perspective(light.position, direction, kAreaShadowAngle, far,
                                        std::max(light.size.x, light.size.y) * light.shadowSoftness));
            break;
        case LightKind3D::kPoint: {
            constexpr std::array<glm::vec3, 6> kFaces{glm::vec3{1, 0, 0},  glm::vec3{-1, 0, 0}, glm::vec3{0, 1, 0},
                                                      glm::vec3{0, -1, 0}, glm::vec3{0, 0, 1},  glm::vec3{0, 0, -1}};
            for (const glm::vec3& face : kFaces) {
                // A little more than 90°, so the faces' edges overlap and the filter finds texels on both.
                tiles.push_back(perspective(light.position, face, 1.62F, far, kBulbSize * light.shadowSoftness));
            }
            break;
        }
    }
    for (ShadowTile& tile : tiles) {
        tile.bias = glm::vec4{light.shadowBias, light.shadowNormalBias, light.shadowBias, 0.0F};
    }
    return tiles;
}

}  // namespace

ShadowPlan planShadows(std::span<const Light3D> lights, const contract::renderer::Environment3D& environment,
                       const ShadowCamera& camera) {
    const std::uint32_t resolution = std::clamp(environment.shadowResolution, 512U, 2048U);
    const std::uint32_t perSide = kShadowAtlasSize / resolution;
    const std::uint32_t capacity = perSide * perSide;
    ShadowPlan plan{.cascades = std::clamp(environment.shadowCascades, 1U, 4U)};
    plan.firstTile.assign(lights.size(), -1);
    std::size_t index = 0;
    for (const Light3D& light : lights) {
        if (light.castShadows) {
            std::vector<ShadowTile> tiles = tilesOf(light, environment, camera, resolution);
            if (plan.tiles.size() + tiles.size() <= capacity) {
                plan.firstTile.at(index) = static_cast<std::int32_t>(plan.tiles.size());
                plan.tiles.insert(plan.tiles.end(), tiles.begin(), tiles.end());
            }
        }
        ++index;
    }
    const float tileSize = 1.0F / static_cast<float>(perSide);
    for (std::size_t tile = 0; tile < plan.tiles.size(); ++tile) {
        ShadowTile& made = plan.tiles.at(tile);
        const std::size_t column = tile % perSide;
        const std::size_t row = tile / perSide;
        made.rect =
            glm::vec4{static_cast<float>(column) * tileSize, static_cast<float>(row) * tileSize, tileSize, tileSize};
        made.size.z = 1.0F / static_cast<float>(kShadowAtlasSize);
    }
    return plan;
}

}  // namespace aylo::render::vulkan::rendering
