/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BuiltinShaders
*/

#include "plugins/shader/BuiltinShaders.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>

namespace aylo::plugins::shader {

namespace {

// The engine's shaders (shader/builtin/*.glsl), embedded by xmake's utils.bin2c rule: null-terminated, ASCII.
constexpr auto kBloom = std::to_array<char>({
#include "bloom.glsl.h"
});
constexpr auto kTonemap = std::to_array<char>({
#include "tonemap.glsl.h"
});

/// @return The text of an embedded file, without its null terminator.
template <std::size_t N>
constexpr std::string_view text(const std::array<char, N>& file) noexcept {
    return {file.data(), N - 1};
}

/// @brief Every shader the engine gives, by the name after kBuiltinShaderPrefix.
constexpr std::array<std::pair<std::string_view, std::string_view>, 2> kShaders{{
    {"bloom.glsl", text(kBloom)},
    {"tonemap.glsl", text(kTonemap)},
}};

}  // namespace

std::optional<std::string_view> findBuiltinShader(std::string_view name) noexcept {
    if (!name.starts_with(kBuiltinShaderPrefix)) {
        return std::nullopt;
    }
    name.remove_prefix(kBuiltinShaderPrefix.size());
    for (const auto& [shaderName, source] : kShaders) {
        if (shaderName == name) {
            return source;
        }
    }
    return std::nullopt;
}

}  // namespace aylo::plugins::shader
