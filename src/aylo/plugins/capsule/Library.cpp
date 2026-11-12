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
#include "plugins/capsule/CapsuleCalls.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    aylo::plugins::capsule::addCapsuleForms(registrar.getPlugins());
    using aylo::plugins::capsule::CapsulePlugin;
    registrar.getPlugins().add(std::string{CapsulePlugin::kName}, [](const aylo::core::config::Settings& /*settings*/) {
        return std::make_unique<CapsulePlugin>();
    });
}
