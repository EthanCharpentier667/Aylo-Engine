/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Overlay
*/

#pragma once

#include <optional>
#include <variant>

#include "aylo/contract/renderer/drawing/Shape.hpp"
#include "aylo/contract/renderer/drawing/Sprite.hpp"
#include "aylo/contract/renderer/drawing/Text.hpp"
#include "aylo/contract/renderer/math/Rect.hpp"

namespace aylo::contract::renderer {

/// @brief One drawing of the overlay, given to IRenderer::drawOverlay().
struct OverlayItem {
    std::variant<Sprite, Shape, Text> drawing;  ///< What to draw, placed in view pixels.
    std::optional<math::Rect> clip;             ///< The part of the view it stays in, in view pixels; nothing: all.
};

}  // namespace aylo::contract::renderer
