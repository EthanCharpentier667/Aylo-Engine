/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Parallax
*/

#pragma once

#include <glm/ext/vector_float2.hpp>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::tilemap {

/// @brief Moves an entity slower or faster than the camera, and scrolls it on its own: the component "aylo.Parallax",
/// for backgrounds and foregrounds.
///
/// @details The TilemapPlugin's Render system "aylo.Tilemap.Parallax" (client only) sets the entity's GlobalTransform2D
/// to its Transform2D, moved by `(camera - origin) * (1 - factor)` and by what it scrolled, `scroll` per second, so
/// far. The camera is the GlobalTransform2D of the Camera2D entity with the highest priority. Works on any entity: a
/// tilemap layer (spawnTilemap() gives each one the parallax factor and scroll Tiled gives it) or a sprite.
/// @code
/// // A far background: moves at a tenth of the camera's speed, and drifts left on its own.
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform2D{})
///     .add(aylo::plugins::sprite::SpriteRenderer{.texture = sky, .layer = -100})
///     .add(aylo::plugins::tilemap::Parallax{.factor = {0.1F, 0.1F}, .scroll = {-8.0F, 0.0F}});
/// @endcode
struct Parallax {
    glm::vec2 factor{1.0F};    ///< 0: stays on screen with the camera; 1: moves with the world; above 1: a foreground.
    glm::vec2 scroll{0.0F};    ///< World units per second it moves on its own.
    glm::vec2 origin{0.0F};    ///< Where the camera sees the entity at its Transform2D, in world units.
    glm::vec2 scrolled{0.0F};  ///< How far it scrolled so far: scroll accumulated over time.
};
AYLO_ECS_COMPONENT(Parallax, "aylo.Parallax", factor, scroll, origin, scrolled);

}  // namespace aylo::plugins::tilemap
