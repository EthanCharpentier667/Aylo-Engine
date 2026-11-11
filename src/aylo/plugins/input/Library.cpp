/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>
#include <string>

#include "aylo/ecs/app/PluginEntry.hpp"
#include "plugins/input/InputCalls.hpp"
#include "plugins/input/InputPlugin.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    aylo::plugins::input::addInputForms(registrar.getPlugins());
    using aylo::plugins::input::InputPlugin;
    registrar.getPlugins().add(std::string{InputPlugin::kName}, [] { return std::make_unique<InputPlugin>(); });
}
