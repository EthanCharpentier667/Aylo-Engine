/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>
#include <string>

#include "aylo/ecs/app/PluginEntry.hpp"
#include "plugins/tilemap/TilemapCalls.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    aylo::plugins::tilemap::addTilemapForms(registrar.getPlugins());
    using aylo::plugins::tilemap::TilemapPlugin;
    registrar.getPlugins().add(std::string{TilemapPlugin::kName}, [] { return std::make_unique<TilemapPlugin>(); });
}
