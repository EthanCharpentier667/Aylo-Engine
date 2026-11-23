/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScreenEffect
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_uint2.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/contract/renderer/Export.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"

namespace aylo::contract::renderer {

/// @brief The name of the picture before any screen effect, as a pass's input: what SCENE_TEXTURE also reads.
inline constexpr std::string_view kSceneTarget = "scene";

/// @brief A texture of a screen effect's material bound to a target of the graph instead of the material's image.
struct ScreenEffectInput {
    std::uint32_t slot{0};  ///< The texture slot of the material's shader (ShaderTexture::slot).
    std::string target;     ///< A target an earlier pass wrote, or kSceneTarget.

    [[nodiscard]] bool operator==(const ScreenEffectInput& other) const = default;
};

/// @brief One pass of the screen effects (IRenderer::setScreenEffects()): a material of a kScreen shader drawn over a
/// target, reading another as TEXTURE.
/// @details A pass with no output writes the chain, at the picture's size: the picture shown is what the last pass
/// writing the chain left, or the scene when none does. A pass with an output writes that target, of `size` if not
/// zero, else `scale` times the picture's size; a later pass reads it by name.
struct ScreenEffectPass {
    MaterialId material;                    ///< Its material, of a shader serving ShaderTarget::kScreen.
    std::string input;                      ///< What it reads as TEXTURE: a target, kSceneTarget, or empty for the
                                            ///< chain as the passes before left it (the scene at first).
    std::vector<ScreenEffectInput> inputs;  ///< Textures of its material bound to targets.
    std::string output;                     ///< The target it writes; empty: the chain.
    float scale{1.0F};                      ///< Its target's size as a share of the picture's.
    glm::uvec2 size{0};                     ///< Its target's size in pixels; 0x0: by scale.
    bool lowPrecision{false};               ///< An 8-bit target (clamped to 0..1) instead of half float.

    [[nodiscard]] bool operator==(const ScreenEffectPass& other) const = default;
};

/// @return Why @p passes cannot run, or nothing: a pass reads a target no earlier pass wrote (or its own), writes the
/// scene, or a target is written twice with different sizes.
[[nodiscard]] AYLO_CONTRACT_RENDERER_API std::optional<std::string> checkScreenEffects(
    std::span<const ScreenEffectPass> passes);

}  // namespace aylo::contract::renderer
