/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BloomPlan
*/

#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>

#include "aylo/contract/renderer/postprocess/PostProcess.hpp"

namespace aylo::render::vulkan::postprocess {

/// @brief Most images in the bloom chain: each is half the size of the previous one, so six reach a glow about
/// 64 pixels wide on the scene, whatever the resolution.
inline constexpr std::uint32_t kMaxBloomLevels = 6;

/// @brief Set in PostConstants::flags for the first downsample, which keeps only the bright part of the scene.
inline constexpr std::uint32_t kFlagPrefilter = 1;

/// @brief The push constants of every post-process shader (shaders/include/PostConstants.glsl): change both together.
/// Each pass reads the fields it needs.
struct PostConstants {
    glm::vec2 texelSize{0.0F};  ///< 1 / size of the image the pass reads, in pixels. Offset 0.
    float threshold = 1.0F;     ///< Brightness from which a colour glows (prefilter). Offset 8.
    float knee = 0.0F;          ///< Width of the soft transition under the threshold, in brightness (prefilter).
    float intensity = 0.0F;     ///< Weight of the glow added back (composite). Offset 16.
    float exposure = 1.0F;      ///< Multiplier before the tonemap (composite). Offset 20.
    std::int32_t tonemap = 0;   ///< contract::renderer::Tonemap (composite). Offset 24.
    std::uint32_t flags = 0;    ///< kFlagPrefilter (first downsample). Offset 28.
};

static_assert(sizeof(PostConstants) == 32, "PostConstants must be the 32 bytes the shaders declare");
static_assert(offsetof(PostConstants, intensity) == 16, "PostConstants::intensity must be at offset 16");
static_assert(offsetof(PostConstants, flags) == 28, "PostConstants::flags must be at offset 28");

/// @return How many images the bloom chain of a scene of @p scene pixels has: it stops before an image gets thinner
/// than 4 pixels, and holds 1 to kMaxBloomLevels.
[[nodiscard]] constexpr std::uint32_t bloomLevelCount(glm::uvec2 scene) noexcept {
    const std::uint32_t side = std::max(std::min(scene.x, scene.y), 1U);
    // Level i is side >> (i + 1) pixels thin: the last one must stay at 4 or more.
    const std::uint32_t fitting = static_cast<std::uint32_t>(std::bit_width(side)) > 3
                                      ? static_cast<std::uint32_t>(std::bit_width(side)) - 3
                                      : 1U;
    return std::clamp(fitting, 1U, kMaxBloomLevels);
}

/// @return The size of bloom image @p level (0 is the first, half the scene): never under 1 pixel.
[[nodiscard]] constexpr glm::uvec2 bloomLevelSize(glm::uvec2 scene, std::uint32_t level) noexcept {
    return glm::uvec2{std::max(scene.x >> (level + 1), 1U), std::max(scene.y >> (level + 1), 1U)};
}

/// @return The constants of the prefilter downsample: reads a @p source image of @p sourceSize pixels.
[[nodiscard]] inline PostConstants makePrefilterConstants(glm::uvec2 sourceSize,
                                                          const contract::renderer::BloomSettings& bloom) noexcept {
    return PostConstants{.texelSize = 1.0F / glm::vec2{sourceSize},
                         .threshold = bloom.threshold,
                         .knee = bloom.threshold * bloom.softKnee,
                         .flags = kFlagPrefilter};
}

/// @return The constants of a plain downsample or upsample reading an image of @p sourceSize pixels.
[[nodiscard]] inline PostConstants makeFilterConstants(glm::uvec2 sourceSize) noexcept {
    return PostConstants{.texelSize = 1.0F / glm::vec2{sourceSize}};
}

/// @return The constants of the composite: @p settings, with the glow weighted 0 when bloom is off.
[[nodiscard]] inline PostConstants makeCompositeConstants(
    const contract::renderer::PostProcessSettings& settings) noexcept {
    return PostConstants{.intensity = settings.bloom.enabled ? settings.bloom.intensity : 0.0F,
                         .exposure = settings.exposure,
                         .tonemap = static_cast<std::int32_t>(settings.tonemap)};
}

}  // namespace aylo::render::vulkan::postprocess
