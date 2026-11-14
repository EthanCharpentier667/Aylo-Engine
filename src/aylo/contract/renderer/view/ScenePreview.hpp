/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScenePreview
*/

#pragma once

#include <glm/ext/vector_uint2.hpp>
#include <optional>
#include <vector>

#include "aylo/contract/renderer/drawing/Lines3D.hpp"
#include "aylo/contract/renderer/view/View.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"

namespace aylo::contract::renderer {

/// @brief A second picture of the frame's world, seen from another point of view, drawn into a texture of its own
/// (IRenderer::setScenePreview()): what a tool shows in a window of its own while the scene goes on showing the game.
/// @details It holds what is drawn in the world (sprites, texts, shapes, particles), not the overlay nor the meshes,
/// placed in its pixels at one world unit per pixel times the view's zoom. With a view3D, the 3D world (lines, static
/// meshes, and the preview's own lines) is drawn first from it, with the depth test, under the 2D world, as in the
/// scene. A renderer that draws no 3D ignores view3D and lines.
struct ScenePreview {
    View view;                     ///< Where it looks at the 2D world from.
    glm::uvec2 size{0};            ///< Its texture's size, in pixels; neither side 0.
    std::optional<View3D> view3D;  ///< Where it looks at the 3D world from; none: no 3D world.
    std::vector<Lines3D> lines;    ///< Lines in the 3D world drawn in the preview only (a tool's grid).
    bool world2D{true};            ///< Whether the 2D world is drawn over the 3D one.
};

}  // namespace aylo::contract::renderer
