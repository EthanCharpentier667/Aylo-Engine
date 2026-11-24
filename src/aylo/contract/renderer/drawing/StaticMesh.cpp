/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StaticMesh
*/

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"

#include <glm/common.hpp>

namespace aylo::contract::renderer {

StaticMeshBounds computeBounds(const StaticMeshDesc& desc) noexcept {
    if (desc.vertices.empty()) {
        return StaticMeshBounds{};
    }
    StaticMeshBounds bounds{.min = desc.vertices.front().position, .max = desc.vertices.front().position};
    for (const StaticMeshVertex& vertex : desc.vertices) {
        bounds.min = glm::min(bounds.min, vertex.position);
        bounds.max = glm::max(bounds.max, vertex.position);
    }
    return bounds;
}

}  // namespace aylo::contract::renderer
