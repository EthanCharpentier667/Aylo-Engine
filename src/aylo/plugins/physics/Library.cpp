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
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    using aylo::plugins::physics::PhysicsPlugin;
    using aylo::plugins::physics::PhysicsSettings;
    registrar.getPlugins().add(std::string{PhysicsPlugin::kName}, [](const aylo::core::config::Settings& settings) {
        return std::make_unique<PhysicsPlugin>(PhysicsSettings::fromSettings(settings.section("physics")));
    });
}
