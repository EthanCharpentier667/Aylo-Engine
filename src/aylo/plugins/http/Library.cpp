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
#include "plugins/http/HttpCalls.hpp"
#include "plugins/http/HttpPlugin.hpp"
#include "plugins/http/HttpSettings.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    aylo::plugins::http::addHttpForms(registrar.getPlugins());
    using aylo::plugins::http::HttpPlugin;
    using aylo::plugins::http::HttpSettings;
    registrar.getPlugins().add(std::string{HttpPlugin::kName}, [](const aylo::core::config::Settings& settings) {
        return std::make_unique<HttpPlugin>(HttpSettings::fromSettings(settings.section("http")));
    });
}
