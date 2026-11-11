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
#include "plugins/json/JsonCalls.hpp"
#include "plugins/json/JsonPlugin.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    using aylo::plugins::json::JsonPlugin;
    aylo::plugins::json::addJsonForms(registrar.getPlugins());
    registrar.getPlugins().add(std::string{JsonPlugin::kName},
                               [](const aylo::core::config::Settings&) { return std::make_unique<JsonPlugin>(); });
}
