/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PostProcessCalls
*/

#pragma once

#include <string_view>

#include "aylo/contract/renderer/postprocess/PostProcess.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/type/String.hpp"
#include "plugins/graphics/Export.hpp"

namespace aylo::plugins::graphics {

inline constexpr std::string_view kSetPostProcessCall = "aylo.backend.setPostProcess";  ///< Sets the PostProcess.

/// @brief The arguments of `aylo.backend.setPostProcess`: the bloom, the exposure and the tonemap of the window. A
/// field left out keeps its default, so a call replaces every setting.
struct SetPostProcessArguments {
    bool bloom{false};            ///< Whether bright colours glow.
    float bloomThreshold{1.0F};   ///< Brightness from which a colour glows; 1 is white. At least 0.
    float bloomKnee{0.5F};        ///< Width of the transition under the threshold, 0 to 1.
    float bloomIntensity{0.35F};  ///< How much of the glow is added back. At least 0.
    ecs::String tonemap{"none"};  ///< "none", "reinhard" or "aces".
    float exposure{1.0F};         ///< Multiplies the colours before the tonemap. At least 0.
};
AYLO_ECS_CALL_DATA(SetPostProcessArguments, "aylo.backend.SetPostProcessArguments", bloom, bloomThreshold, bloomKnee,
                   bloomIntensity, tonemap, exposure);

/// @return The settings @p arguments ask for, brought into range (PostProcessSettings::sanitized()).
/// @throws exceptions::PostProcessCallException If the tonemap has no such name.
[[nodiscard]] AYLO_PLUGIN_GRAPHICS_API contract::renderer::PostProcessSettings toPostProcessSettings(
    const SetPostProcessArguments& arguments);

/// @brief Registers the post-process engine call on @p app, for scripts: `aylo.backend.setPostProcess`, which writes
/// the `aylo.PostProcess` resource; the GraphicsPlugin hands it to the renderer each frame it changes. Called by the
/// GraphicsPlugin.
void addPostProcessCalls(ecs::App& app);

}  // namespace aylo::plugins::graphics
