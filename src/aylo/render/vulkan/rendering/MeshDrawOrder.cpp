/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshDrawOrder
*/

#include "MeshDrawOrder.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <iterator>
#include <span>
#include <vector>

namespace aylo::render::vulkan::rendering {

namespace {

/// @return How far @p center is from @p viewpoint, to sort by: larger is farther.
float farness(const glm::vec3& center, const MeshViewpoint& viewpoint) noexcept {
    const glm::vec3 offset = center - viewpoint.position;
    return viewpoint.perspective ? glm::dot(offset, offset) : glm::dot(offset, viewpoint.forward);
}

}  // namespace

void orderMeshDraws(std::span<const MeshDrawKey> draws, const MeshViewpoint& viewpoint,
                    std::vector<std::uint32_t>& order) {
    order.clear();
    std::uint32_t index = 0;
    for (const MeshDrawKey& draw : draws) {
        if (!draw.transparent) {
            order.push_back(index);
        }
        ++index;
    }
    const auto firstBlended = static_cast<std::ptrdiff_t>(order.size());
    index = 0;
    for (const MeshDrawKey& draw : draws) {
        if (draw.transparent) {
            order.push_back(index);
        }
        ++index;
    }
    std::ranges::stable_sort(
        std::next(order.begin(), firstBlended), order.end(), std::ranges::greater{},
        [&draws, &viewpoint](std::uint32_t draw) { return farness(draws.subspan(draw, 1).front().center, viewpoint); });
}

}  // namespace aylo::render::vulkan::rendering
