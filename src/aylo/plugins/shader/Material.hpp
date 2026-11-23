/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Material
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/shader/Export.hpp"

namespace aylo::plugins::shader {

/// @brief A value of a material, of one of the types a shader declares: float, vec2, vec3, vec4 or int.
using MaterialValue = std::variant<float, glm::vec2, glm::vec3, glm::vec4, std::int32_t>;

/// @brief What a material changes of its 3D shader's render state (contract::renderer::RenderState): each part it
/// sets replaces the shader's pragma, the others keep it.
struct RenderStateOverride {
    std::optional<contract::renderer::CullMode> cull;    ///< Replaces `#pragma cull`.
    std::optional<contract::renderer::MeshBlend> blend;  ///< Replaces `#pragma blend`.
    std::optional<contract::renderer::DepthMode> depth;  ///< Replaces `#pragma depth`.
    std::optional<float> depthBias;                      ///< Replaces the first number of `#pragma depth_bias`.
    std::optional<float> depthBiasSlope;                 ///< Replaces its slope factor.

    /// @return Whether it changes nothing.
    [[nodiscard]] bool empty() const noexcept { return !cull && !blend && !depth && !depthBias && !depthBiasSlope; }

    /// @return @p state with what this sets replaced. A blend set to a blended mode without a depth mode, over an
    /// opaque shader's default, stops writing depth, as the pragma does.
    [[nodiscard]] contract::renderer::RenderState applyTo(contract::renderer::RenderState state) const noexcept {
        const bool wasDefaultDepth = state.depth == contract::renderer::DepthMode::kTestWrite;
        state.cull = cull.value_or(state.cull);
        state.blend = blend.value_or(state.blend);
        state.depth = depth.value_or(state.depth);
        if (!depth && blend && wasDefaultDepth && contract::renderer::isTransparent(state)) {
            state.depth = contract::renderer::DepthMode::kTestNoWrite;
        }
        state.depthBias = depthBias.value_or(state.depthBias);
        state.depthBiasSlope = depthBiasSlope.value_or(state.depthBiasSlope);
        return state;
    }

    [[nodiscard]] bool operator==(const RenderStateOverride& other) const = default;
};

/// @brief A shader and the values of its `uniform`s and textures, by name: what a game describes, shared by every
/// drawing that uses it. The values it does not set keep the shader's defaults.
/// @code
/// aylo::plugins::shader::Material hit{.shader = shaders->load("assets/shaders/flash.glsl")};
/// hit.set("tint", glm::vec4{1.0F, 0.2F, 0.2F, 1.0F}).setTexture("noise", textures->load("assets/noise.png"));
/// @endcode
struct AYLO_PLUGIN_SHADER_API Material {
    /// @brief One value set by name.
    struct Value {
        std::string name;     ///< As declared in the shader.
        MaterialValue value;  ///< Its type must be the declared one.
    };

    /// @brief One texture set by name.
    struct Texture {
        std::string name;              ///< As declared in the shader (`uniform sampler2D <name>;`).
        core::asset::AssetId texture;  ///< From TextureAssets::load().
    };

    core::asset::AssetId shader;      ///< From ShaderAssets::load() or add().
    std::vector<Value> values;        ///< The values set; a name set twice keeps the last.
    std::vector<Texture> textures;    ///< The textures set; a name set twice keeps the last.
    RenderStateOverride renderState;  ///< What it changes of its 3D shader's render state.

    /// @brief Sets the value @p name, replacing a previous one.
    /// @return This material, to chain.
    Material& set(const std::string& name, MaterialValue value);

    /// @brief Sets the texture @p name, replacing a previous one.
    /// @return This material, to chain.
    Material& setTexture(const std::string& name, core::asset::AssetId texture);
};

/// @return The material block of @p material for @p shader: the shader's defaults, with every value @p material sets
/// written at its offset. CompiledShader::materialSize bytes.
/// @throws exceptions::ShaderException If a value has no `uniform` of this name in the shader (an `instance uniform`
/// is set per drawing, not in a material) or is not of its type.
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::vector<std::byte> packMaterialValues(
    const contract::renderer::CompiledShader& shader, const Material& material);

/// @brief Writes @p value at the place of @p shader's `instance uniform` @p name in @p values.
/// @throws exceptions::ShaderException If @p shader has no `instance uniform` of this name (a `uniform` is set in a
/// material) or @p value is not of its type.
AYLO_PLUGIN_SHADER_API void writeInstanceValue(const contract::renderer::CompiledShader& shader,
                                               contract::renderer::ShaderInstanceValues& values,
                                               const std::string& name, const MaterialValue& value);

/// @return The textures of @p material for @p shader, by slot; an unset slot keeps a default AssetId.
/// @throws exceptions::ShaderException If a texture has no `uniform sampler2D` of this name in the shader.
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::array<core::asset::AssetId, contract::renderer::kShaderMaxTextures>
resolveMaterialTextures(const contract::renderer::CompiledShader& shader, const Material& material);

}  // namespace aylo::plugins::shader
