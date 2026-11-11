/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PhysicsDebugSettings
*/

#pragma once

#include "aylo/contract/platform/input/Key.hpp"
#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/plugins/physics/debug/Export.hpp"
#include "core/config/Settings.hpp"

namespace aylo::plugins::physics::debug {

/// @brief How the PhysicsDebugPlugin draws: the global switches next to the per-entity PhysicsDebug component.
/// @details Inserted with its defaults by the plugin if the App has none; insert one before or after the plugin to
/// change them.
/// @code
/// app.insertResource(aylo::plugins::physics::debug::PhysicsDebugSettings{.drawAll = true});  // Every collider, F3
/// toggles.
/// @endcode
struct PhysicsDebugSettings {
    bool enabled = true;   ///< Draws nothing when false; the toggle key flips it.
    bool drawAll = false;  ///< Draws every collider and speed, PhysicsDebug or not.
    contract::platform::input::Key toggleKey =
        contract::platform::input::Key::kF3;                  ///< Flips enabled; kUnknown: no key.
    contract::renderer::Color color{0.2F, 1.0F, 0.4F, 1.0F};  ///< For entities drawn by drawAll alone.
    float outlineThickness = 1.5F;                            ///< Of outlines and arrows, in world units.
    float velocityScale = 0.1F;  ///< Seconds of travel an arrow shows: a speed of 300 draws a 30 units long arrow.

    /// @brief Reads the keys `enabled`, `drawAll`, `outlineThickness` and `velocityScale`; the missing ones, and the
    /// toggle key and the color, keep their default.
    /// @param settings The `physicsDebug` section.
    /// @throws core::config::exceptions::SettingsException If a key is unknown, has the wrong type or an invalid value.
    [[nodiscard]] AYLO_PLUGIN_PHYSICS_DEBUG_API static PhysicsDebugSettings fromSettings(
        const core::config::Settings& settings);
};
AYLO_ECS_RESOURCE(PhysicsDebugSettings, "rtype.PhysicsDebugSettings");

}  // namespace aylo::plugins::physics::debug
