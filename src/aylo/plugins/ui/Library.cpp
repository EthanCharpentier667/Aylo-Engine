/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>
#include <string>

#include "aylo/ecs/app/PluginEntry.hpp"
#include "plugins/ui/UiPlugin.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    using aylo::plugins::ui::UiPlugin;
    aylo::plugins::ui::addUiForms(registrar.getPlugins());
    registrar.getPlugins().add(std::string{UiPlugin::kName}, [] { return std::make_unique<UiPlugin>(); });
}
