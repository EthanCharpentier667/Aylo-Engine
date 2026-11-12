/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>
#include <string>

#include "aylo/ecs/app/PluginEntry.hpp"
#include "plugins/particle/ParticleCalls.hpp"
#include "plugins/particle/ParticlePlugin.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    aylo::plugins::particle::addParticleForms(registrar.getPlugins());
    using aylo::plugins::particle::ParticlePlugin;
    registrar.getPlugins().add(std::string{ParticlePlugin::kName}, [] { return std::make_unique<ParticlePlugin>(); });
}
