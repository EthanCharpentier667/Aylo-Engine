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
#include "plugins/persistence/PersistenceCalls.hpp"
#include "plugins/persistence/PersistencePlugin.hpp"
#include "plugins/persistence/PersistenceSettings.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    using aylo::plugins::persistence::PersistencePlugin;
    using aylo::plugins::persistence::PersistenceSettings;
    aylo::plugins::persistence::addPersistenceForms(registrar.getPlugins());
    registrar.getPlugins().add(std::string{PersistencePlugin::kName}, [](const aylo::core::config::Settings& settings) {
        return std::make_unique<PersistencePlugin>(
            PersistenceSettings::fromSettings(settings.section("persistence"), settings.getString("name", "")));
    });
}
