/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScreenMaterials
*/

#pragma once

#include <glm/ext/vector_uint2.hpp>
#include <string>
#include <utility>
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
    /// @brief One pass of the screen effects as a script gives it: names, not the renderer's ids and slots.
    struct Pass {
        core::asset::AssetId material;                            ///< A material of a `#pragma screen` shader.
        std::string input;                                        ///< The target it reads as TEXTURE; empty: the chain.
        std::vector<std::pair<std::string, std::string>> inputs;  ///< A `uniform sampler2D` of its shader, by name,
                                                                  ///< bound to a target.
        std::string output;                                       ///< The target it writes; empty: the chain.
        float scale{1.0F};                                        ///< Its target's share of the screen's size.
        glm::uvec2 size{0};                                       ///< Its target's size in pixels; 0x0: by scale.
        bool lowPrecision{false};                                 ///< An 8-bit target.
    };

    std::vector<core::asset::AssetId> materials;  ///< In the order they run: each reads what the previous one wrote.
    std::vector<Pass> passes;  ///< Passes, by `aylo.shader.addScreenEffect`: used instead of materials when not empty.
};
AYLO_ECS_RESOURCE(ScreenMaterials, "aylo.ScreenMaterials");

}  // namespace aylo::plugins::shader
