/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>
#include <string>

#include "aylo/ecs/app/PluginEntry.hpp"
#include "plugins/mesh/MeshCalls.hpp"
#include "plugins/mesh/MeshPlugin.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    aylo::plugins::mesh::addMeshForms(registrar.getPlugins());
    using aylo::plugins::mesh::MeshPlugin;
    registrar.getPlugins().add(std::string{MeshPlugin::kName}, [] { return std::make_unique<MeshPlugin>(); });
}
