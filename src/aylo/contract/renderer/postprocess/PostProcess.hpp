/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PostProcess
*/

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace aylo::contract::renderer {

/// @brief How the HDR colours of a frame are brought back to what a screen shows.
enum class Tonemap : std::uint8_t {
    kNone,      ///< Clamped to 0..1: the picture of a renderer without post-processing.
    kReinhard,  ///< `c / (1 + c)`: soft, never reaches white.
    kAces,      ///< An approximation of the ACES filmic curve: contrasted, reaches white.
};

/// @brief The glow around what is brighter than white (lasers, explosions).
struct BloomSettings {
    bool enabled = false;     ///< Whether the glow is added to the frame.
    float threshold = 1.0F;   ///< Brightness from which a colour glows; 1 is white. At least 0.
    float softKnee = 0.5F;    ///< Width of the transition under the threshold, 0 (hard edge) to 1. Clamped.
    float intensity = 0.35F;  ///< How much of the glow is added back. At least 0.

    friend bool operator==(const BloomSettings&, const BloomSettings&) = default;
};

/// @brief What happens to the frame between the scene and the window.
///
/// @details The scene is drawn in a floating-point target, so a colour may exceed 1 (a shader that writes
/// `COLOR.rgb * 4.0`): bloom lets it glow and a tonemap decides how it is shown. The defaults change nothing of the
/// picture. A renderer applies it from the next frame it draws; see IRenderer::setPostProcess().
struct PostProcessSettings {
    BloomSettings bloom;               ///< The glow.
    Tonemap tonemap = Tonemap::kNone;  ///< How HDR colours are shown.
    float exposure = 1.0F;             ///< Multiplies the colours before the tonemap. At least 0.

    /// @return These settings with every value brought into its range: a negative or NaN number becomes the closest
    /// valid one, so a renderer never has to check them.
    [[nodiscard]] PostProcessSettings sanitized() const noexcept {
        const auto atLeastZero = [](float value, float fallback) {
            return std::isfinite(value) ? std::max(value, 0.0F) : fallback;
        };
        PostProcessSettings result = *this;
        result.bloom.threshold = atLeastZero(bloom.threshold, BloomSettings{}.threshold);
        result.bloom.softKnee = std::clamp(atLeastZero(bloom.softKnee, BloomSettings{}.softKnee), 0.0F, 1.0F);
        result.bloom.intensity = atLeastZero(bloom.intensity, BloomSettings{}.intensity);
        result.exposure = atLeastZero(exposure, PostProcessSettings{}.exposure);
        return result;
    }

    friend bool operator==(const PostProcessSettings&, const PostProcessSettings&) = default;
};

}  // namespace aylo::contract::renderer
