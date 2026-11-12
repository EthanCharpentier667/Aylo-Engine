/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** HotReload
*/

#pragma once

#include "aylo/core/reload/HotReloader.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/resource/Resources.hpp"

namespace aylo::plugins {

/// @brief What lets a plugin follow its files while the game runs, as the ECS resource "aylo.HotReload" the Engine
/// inserts before the plugins are built, when hot reload is on: the engine's core::reload::HotReloader. A plugin adds
/// a core::reload::Reloadable to it when it is built, and the Engine looks at the files between two frames. A program
/// without an Engine, or with hot reload off, has none.
///
/// @details The reloader is the Engine's and outlives the App; the Engine forgets what a plugin added before it builds
/// the App again, so a Reloadable may keep a pointer into the App.
struct HotReload {
    core::reload::HotReloader* reloader{nullptr};  ///< The reloader to add to; never null in the resource.
};
AYLO_ECS_RESOURCE(HotReload, "aylo.HotReload");

/// @return The reloader @p resources hold, or null if hot reload is off.
[[nodiscard]] inline core::reload::HotReloader* hotReloaderOf(const ecs::Resources& resources) {
    const auto* hotReload = resources.get<HotReload>();
    return hotReload != nullptr ? hotReload->reloader : nullptr;
}

}  // namespace aylo::plugins
