/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>
#include <string>

#include "aylo/ecs/app/PluginEntry.hpp"
#include "plugins/sprite/SpriteCalls.hpp"
#include "plugins/sprite/SpriteDrawPlugin.hpp"
#include "plugins/sprite/SpritePlugin.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    aylo::plugins::sprite::addSpriteForms(registrar.getPlugins());
    using aylo::plugins::sprite::SpriteDrawPlugin;
    using aylo::plugins::sprite::SpritePlugin;
    registrar.getPlugins().add(std::string{SpritePlugin::kName}, [] { return std::make_unique<SpritePlugin>(); });
    registrar.getPlugins().add(std::string{SpriteDrawPlugin::kName},
                               [] { return std::make_unique<SpriteDrawPlugin>(); });
}
