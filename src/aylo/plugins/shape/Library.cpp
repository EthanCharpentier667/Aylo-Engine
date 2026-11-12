/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>
#include <string>

#include "aylo/ecs/app/PluginEntry.hpp"
#include "plugins/shape/ShapePlugin.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    using aylo::plugins::shape::ShapePlugin;
    registrar.getPlugins().add(std::string{ShapePlugin::kName}, [] { return std::make_unique<ShapePlugin>(); });
}
