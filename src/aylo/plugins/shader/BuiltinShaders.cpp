/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BuiltinShaders
*/

#include "plugins/shader/BuiltinShaders.hpp"

#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
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
constexpr auto kBloomPrefilter = std::to_array<char>({
#include "bloom_prefilter.glsl.h"
});
constexpr auto kBloomDown = std::to_array<char>({
#include "bloom_down.glsl.h"
});
constexpr auto kBloomUp = std::to_array<char>({
#include "bloom_up.glsl.h"
});
constexpr auto kBloomComposite = std::to_array<char>({
#include "bloom_composite.glsl.h"
});
constexpr auto kBlur = std::to_array<char>({
#include "blur.glsl.h"
});
constexpr auto kKuwahara = std::to_array<char>({
#include "kuwahara.glsl.h"
});
constexpr auto kKuwaharaTensor = std::to_array<char>({
#include "kuwahara_tensor.glsl.h"
});
constexpr auto kKuwaharaAnisotropic = std::to_array<char>({
#include "kuwahara_anisotropic.glsl.h"
});
constexpr auto kSharpen = std::to_array<char>({
#include "sharpen.glsl.h"
});
constexpr auto kChromaticAberration = std::to_array<char>({
#include "chromatic_aberration.glsl.h"
});
constexpr auto kFilmGrain = std::to_array<char>({
#include "film_grain.glsl.h"
});
constexpr auto kVignette = std::to_array<char>({
#include "vignette.glsl.h"
});
constexpr auto kFxaa = std::to_array<char>({
#include "fxaa.glsl.h"
});
constexpr auto kSsao = std::to_array<char>({
#include "ssao.glsl.h"
});
constexpr auto kSsaoApply = std::to_array<char>({
#include "ssao_apply.glsl.h"
});
constexpr auto kFog = std::to_array<char>({
#include "fog.glsl.h"
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
constexpr std::array<std::pair<std::string_view, std::string_view>, 18> kShaders{{
    {"bloom.glsl", text(kBloom)},
    {"tonemap.glsl", text(kTonemap)},
    {"bloom_prefilter.glsl", text(kBloomPrefilter)},
    {"bloom_down.glsl", text(kBloomDown)},
    {"bloom_up.glsl", text(kBloomUp)},
    {"bloom_composite.glsl", text(kBloomComposite)},
    {"blur.glsl", text(kBlur)},
    {"kuwahara.glsl", text(kKuwahara)},
    {"kuwahara_tensor.glsl", text(kKuwaharaTensor)},
    {"kuwahara_anisotropic.glsl", text(kKuwaharaAnisotropic)},
    {"sharpen.glsl", text(kSharpen)},
    {"chromatic_aberration.glsl", text(kChromaticAberration)},
    {"film_grain.glsl", text(kFilmGrain)},
    {"vignette.glsl", text(kVignette)},
    {"fxaa.glsl", text(kFxaa)},
    {"ssao.glsl", text(kSsao)},
    {"ssao_apply.glsl", text(kSsaoApply)},
    {"fog.glsl", text(kFog)},
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

std::string listBuiltinShaders() {
    std::string names;
    for (const auto& [name, source] : kShaders) {
        names += std::format("{}{}{}", names.empty() ? "" : ", ", kBuiltinShaderPrefix, name);
    }
    return names;
}

std::optional<std::string_view> findBuiltinLibrary(std::string_view name) noexcept { return findIn(kLibraries, name); }

}  // namespace aylo::plugins::shader
