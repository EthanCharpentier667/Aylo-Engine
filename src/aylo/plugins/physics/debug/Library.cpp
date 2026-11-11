/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>
#include <string>

#include "aylo/ecs/app/PluginEntry.hpp"
#include "core/config/Settings.hpp"
#include "plugins/physics/debug/PhysicsDebugPlugin.hpp"
#include "plugins/physics/debug/PhysicsDebugSettings.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    using aylo::plugins::physics::debug::PhysicsDebugPlugin;
    using aylo::plugins::physics::debug::PhysicsDebugSettings;
    registrar.getPlugins().add(std::string{PhysicsDebugPlugin::kName},
                               [](const aylo::core::config::Settings& settings) {
                                   return std::make_unique<PhysicsDebugPlugin>(
                                       PhysicsDebugSettings::fromSettings(settings.section("physicsDebug")));
                               });
}
