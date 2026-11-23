/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RenderStateNames
*/

#pragma once

#include <optional>
#include <span>
#include <string_view>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "plugins/shader/Export.hpp"

namespace aylo::plugins::shader {

/// @brief The names of a 3D material's render state, as the pragmas, the `.material` files, the scripts and the
/// editor write them: `"back"`, `"front"`, `"none"`; `"opaque"`, `"alpha"`, `"premultiplied"`, `"additive"`,
/// `"multiply"`; `"test write"`, `"test nowrite"`, `"notest"`.
/// @{
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::string_view nameOf(contract::renderer::CullMode mode) noexcept;
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::string_view nameOf(contract::renderer::MeshBlend mode) noexcept;
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::string_view nameOf(contract::renderer::DepthMode mode) noexcept;
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::optional<contract::renderer::CullMode> cullModeNamed(
    std::string_view name) noexcept;
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::optional<contract::renderer::MeshBlend> blendNamed(
    std::string_view name) noexcept;
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::optional<contract::renderer::DepthMode> depthModeNamed(
    std::string_view name) noexcept;
/// @return Every name of a kind, in the order of its enum, for a list to pick from.
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::span<const std::string_view> cullModeNames() noexcept;
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::span<const std::string_view> blendNames() noexcept;
[[nodiscard]] AYLO_PLUGIN_SHADER_API std::span<const std::string_view> depthModeNames() noexcept;
/// @}

}  // namespace aylo::plugins::shader
