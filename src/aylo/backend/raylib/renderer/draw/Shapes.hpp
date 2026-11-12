/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Shapes
*/

#pragma once

#include "aylo/contract/renderer/drawing/Shape.hpp"

namespace aylo::backend::raylib::renderer::draw {

/// @brief Draws @p shape with raylib's shape functions: its fill, then its outline, drawn inside the shape.
/// @details A rotated rectangle loses its rounded corners: raylib only rounds rectangles that are not turned.
void drawShape(const contract::renderer::Shape& shape);

}  // namespace aylo::backend::raylib::renderer::draw
