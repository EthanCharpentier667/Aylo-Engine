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
constexpr auto kNoise = std::to_array<char>({
#include "noise.glsl.h"
});
constexpr auto kColor = std::to_array<char>({
#include "color.glsl.h"
});
constexpr auto kMath = std::to_array<char>({
#include "math.glsl.h"
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

/// @brief Every library the engine gives, by the name after kBuiltinShaderPrefix.
constexpr std::array<std::pair<std::string_view, std::string_view>, 3> kLibraries{{
    {"noise.glsl", text(kNoise)},
    {"color.glsl", text(kColor)},
    {"math.glsl", text(kMath)},
}};

/// @return The file of @p files that @p name (with kBuiltinShaderPrefix) names.
template <std::size_t N>
std::optional<std::string_view> findIn(const std::array<std::pair<std::string_view, std::string_view>, N>& files,
                                       std::string_view name) noexcept {
    if (!name.starts_with(kBuiltinShaderPrefix)) {
        return std::nullopt;
    }
    name.remove_prefix(kBuiltinShaderPrefix.size());
    for (const auto& [fileName, source] : files) {
        if (fileName == name) {
            return source;
        }
    }
    return std::nullopt;
}

}  // namespace

std::optional<std::string_view> findBuiltinShader(std::string_view name) noexcept { return findIn(kShaders, name); }

std::optional<std::string_view> findBuiltinLibrary(std::string_view name) noexcept { return findIn(kLibraries, name); }

}  // namespace aylo::plugins::shader
