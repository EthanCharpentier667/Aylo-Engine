/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>
#include <string>

#include "aylo/ecs/app/PluginEntry.hpp"
#include "plugins/transform/TransformPlugin.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    using aylo::plugins::transform::TransformPlugin;
    registrar.getPlugins().add(std::string{TransformPlugin::kName}, [] { return std::make_unique<TransformPlugin>(); });
}
