/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CapsuleCalls
*/

#pragma once

#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/capsule/Export.hpp"

namespace aylo::plugins::capsule {
inline constexpr std::string_view kSpawnCall = "aylo.capsule.spawn";    ///< spawnCapsule().
inline constexpr std::string_view kLoadCall = "aylo.capsule.load";      ///< loadCapsule(): switches levels.
inline constexpr std::string_view kReloadCall = "aylo.capsule.reload";  ///< reloadCapsule().

/// @brief The arguments of `aylo.capsule.spawn` and `aylo.capsule.load`: the capsule, by path from a script
/// ("actors/bydo.actor").
struct CapsuleArguments {
    core::asset::AssetId capsule;  ///< The capsule.
};
AYLO_ECS_CALL_DATA(CapsuleArguments, "aylo.capsule.CapsuleArguments", capsule);

/// @brief The result of `aylo.capsule.spawn` and `aylo.capsule.load`: the root of the capsule spawned.
struct CapsuleRoot {
    ecs::Entity root;  ///< The root, reserved at once, alive when the system's commands are applied.
};
AYLO_ECS_CALL_DATA(CapsuleRoot, "aylo.capsule.CapsuleRoot", root);

/// @brief The arguments of `aylo.capsule.reload`.
struct ReloadArguments {
    ecs::Entity root;  ///< The root of a spawned capsule.
};
AYLO_ECS_CALL_DATA(ReloadArguments, "aylo.capsule.ReloadArguments", root);

/// @brief The result of `aylo.capsule.reload`.
struct ReloadResult {
    bool reloaded{false};  ///< False when `root` is not the root of a spawned capsule (or is gone).
};
AYLO_ECS_CALL_DATA(ReloadResult, "aylo.capsule.ReloadResult", reloaded);

/// @brief Registers the capsule engine calls on @p app, for scripts: `aylo.capsule.spawn` (on `aylo.Capsules`),
/// `aylo.capsule.load` (also writes `aylo.LoadedCapsule`) and `aylo.capsule.reload` (also reads the
/// `aylo.CapsuleInstance` and `aylo.Children` components), each recording into the commands of the system that calls
/// it. They throw a CapsuleException naming a capsule that cannot be spawned. Called by the CapsulePlugin.
AYLO_PLUGIN_CAPSULE_API void addCapsuleCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin on @p registry: `engine.capsule.spawn`, `load` and `reload`, which
/// make the calls above from a system's `ctx` and patch the root with overrides. Its library does it on every side,
/// and so does building the plugin, for a program without libraries.
AYLO_PLUGIN_CAPSULE_API void addCapsuleForms(ecs::PluginRegistry& registry);
}  // namespace aylo::plugins::capsule
