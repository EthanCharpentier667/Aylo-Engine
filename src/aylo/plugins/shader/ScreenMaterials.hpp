/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScreenMaterials
*/

#pragma once

#include <vector>

#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "core/asset/AssetId.hpp"

namespace aylo::plugins::shader {

/// @brief The screen effects of the game, as the client resource "aylo.ScreenMaterials" the ShaderPlugin inserts:
/// materials (MaterialAssets ids) of shaders that say `#pragma screen`, run in this order over the scene once the world
/// is drawn. Set by `aylo.shader.setScreenMaterials`; the ShaderPlugin hands the ones created in the renderer to it
/// every frame (IRenderer::setScreenMaterials()), so a material reloaded or created late follows. Empty: the scene is
/// shown as drawn.
struct ScreenMaterials {
    std::vector<core::asset::AssetId> materials;  ///< In the order they run: each reads what the previous one wrote.
};
AYLO_ECS_RESOURCE(ScreenMaterials, "aylo.ScreenMaterials");

}  // namespace aylo::plugins::shader
