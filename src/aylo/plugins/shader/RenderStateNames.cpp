/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RenderStateNames
*/

#include "plugins/shader/RenderStateNames.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"

namespace aylo::plugins::shader {

namespace {

using contract::renderer::CullMode;
using contract::renderer::DepthMode;
using contract::renderer::MeshBlend;

/// @brief The names of each kind, in the order of its enum.
constexpr std::array<std::string_view, 3> kCullNames{"back", "front", "none"};
constexpr std::array<std::string_view, 5> kBlendNames{"opaque", "alpha", "premultiplied", "additive", "multiply"};
constexpr std::array<std::string_view, 3> kDepthNames{"test write", "test nowrite", "notest"};

/// @return The name of the value @p index of a kind named by @p names, or an empty name past its end.
template <std::size_t N>
std::string_view nameAt(const std::array<std::string_view, N>& names, std::size_t index) noexcept {
    return index < names.size() ? std::span{names}.subspan(index).front() : std::string_view{};
}

/// @return The enum value named @p name among @p names, or nothing.
template <typename Enum, std::size_t N>
std::optional<Enum> valueNamed(const std::array<std::string_view, N>& names, std::string_view name) noexcept {
    for (std::size_t index = 0; index < names.size(); ++index) {
        if (std::span{names}.subspan(index).front() == name) {
            return static_cast<Enum>(index);
        }
    }
    return std::nullopt;
}

}  // namespace

std::string_view nameOf(CullMode mode) noexcept { return nameAt(kCullNames, static_cast<std::size_t>(mode)); }
std::string_view nameOf(MeshBlend mode) noexcept { return nameAt(kBlendNames, static_cast<std::size_t>(mode)); }
std::string_view nameOf(DepthMode mode) noexcept { return nameAt(kDepthNames, static_cast<std::size_t>(mode)); }

std::optional<CullMode> cullModeNamed(std::string_view name) noexcept { return valueNamed<CullMode>(kCullNames, name); }
std::optional<MeshBlend> blendNamed(std::string_view name) noexcept { return valueNamed<MeshBlend>(kBlendNames, name); }
std::optional<DepthMode> depthModeNamed(std::string_view name) noexcept {
    return valueNamed<DepthMode>(kDepthNames, name);
}

std::span<const std::string_view> cullModeNames() noexcept { return kCullNames; }
std::span<const std::string_view> blendNames() noexcept { return kBlendNames; }
std::span<const std::string_view> depthModeNames() noexcept { return kDepthNames; }

}  // namespace aylo::plugins::shader
