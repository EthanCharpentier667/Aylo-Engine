/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScenePreview
*/

#pragma once

#include <glm/ext/vector_uint2.hpp>

#include "aylo/contract/renderer/view/View.hpp"

namespace aylo::contract::renderer {

/// @brief A second picture of the frame's world, seen from another point of view, drawn into a texture of its own
/// (IRenderer::setScenePreview()): what a tool shows in a window of its own while the scene goes on showing the game.
/// @details It holds what is drawn in the world (sprites, texts, shapes, particles), not the overlay nor the meshes,
/// placed in its pixels at one world unit per pixel times the view's zoom.
struct ScenePreview {
    View view;           ///< Where it looks from.
    glm::uvec2 size{0};  ///< Its texture's size, in pixels; neither side 0.
};

}  // namespace aylo::contract::renderer
