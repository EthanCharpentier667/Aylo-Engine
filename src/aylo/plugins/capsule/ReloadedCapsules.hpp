/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ReloadedCapsules
*/

#pragma once

#include <vector>

#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "plugins/capsule/Capsule.hpp"

namespace aylo::plugins::capsule {
/// @brief The capsules changed since the last frame, the "aylo.ReloadedCapsules" resource the CapsulePlugin inserts:
/// the CapsuleReloadable fills it between two frames with the files hot reload read again, a tool (the editor) with
/// the capsules it redefined (Capsules::define()), and the plugin's Render system "aylo.Capsule.HotReload" empties it,
/// reloading what was spawned from them.
struct ReloadedCapsules {
    std::vector<CapsuleId> capsules;  ///< Read again, not reloaded in the world yet.
};
AYLO_ECS_RESOURCE(ReloadedCapsules, "aylo.ReloadedCapsules");
}  // namespace aylo::plugins::capsule
