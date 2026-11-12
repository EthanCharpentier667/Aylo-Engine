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
#include "plugins/shader/ShaderCalls.hpp"
#include "plugins/shader/ShaderPlugin.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    aylo::plugins::shader::addShaderForms(registrar.getPlugins());
    using aylo::plugins::shader::ShaderPlugin;
    registrar.getPlugins().add(std::string{ShaderPlugin::kName}, [](const aylo::core::config::Settings& /*settings*/) {
        return std::make_unique<ShaderPlugin>();
    });
}
