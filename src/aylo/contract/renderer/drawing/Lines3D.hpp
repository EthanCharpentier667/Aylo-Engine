/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Lines3D
*/

#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <vector>

#include "aylo/contract/renderer/drawing/Color.hpp"

namespace aylo::contract::renderer {

/// @brief Segments in the 3D world, drawn one pixel wide with the depth test, by IRenderer::drawLines3D():
/// grids, gizmos, the physics debug view.
/// @details Points go in pairs, each pair one segment (an odd last point is ignored). They are placed by @p world,
/// then seen through the frame's View3D. Drawn only by a renderer whose supports3D() is true, in a frame with a View3D.
struct Lines3D {
    std::vector<glm::vec3> points;  ///< Two per segment, in the space @p world places.
    Color color{1.0F};              ///< Linear RGBA, not premultiplied: the whole set's colour.
    glm::mat4 world{1.0F};          ///< Places the points in the world.
};

}  // namespace aylo::contract::renderer
