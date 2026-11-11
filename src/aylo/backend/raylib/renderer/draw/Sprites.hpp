/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Sprites
*/

#pragma once

#include <raylib.h>

#include "aylo/contract/renderer/drawing/Sprite.hpp"

namespace aylo::backend::raylib::renderer::draw {

/// @brief Draws @p sprite with its @p texture: its source rectangle (all of the texture if empty), scaled, turned
/// around its origin and tinted. A negative scale mirrors it, as raylib mirrors a source rectangle of negative size.
void drawSprite(const ::Texture2D& texture, const contract::renderer::Sprite& sprite);

}  // namespace aylo::backend::raylib::renderer::draw
