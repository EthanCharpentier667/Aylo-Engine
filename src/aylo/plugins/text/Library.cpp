/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>
#include <string>

#include "aylo/ecs/app/PluginEntry.hpp"
#include "plugins/text/TextCalls.hpp"
#include "plugins/text/TextPlugin.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    aylo::plugins::text::addTextForms(registrar.getPlugins());
    using aylo::plugins::text::TextPlugin;
    registrar.getPlugins().add(std::string{TextPlugin::kName}, [] { return std::make_unique<TextPlugin>(); });
}
