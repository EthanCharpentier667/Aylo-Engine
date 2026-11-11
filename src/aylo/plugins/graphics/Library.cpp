/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "aylo/contract/platform/PlatformRegistry.hpp"
#include "aylo/contract/renderer/RendererRegistry.hpp"
#include "aylo/ecs/app/PluginEntry.hpp"
#include "core/config/Settings.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/TextureCalls.hpp"

AYLO_PLUGIN_LIBRARY(registrar) {
    aylo::plugins::graphics::addTextureForms(registrar.getPlugins());
    using aylo::plugins::graphics::BackendRegistry;
    using aylo::plugins::graphics::GraphicsPlugin;
    auto backends = std::make_shared<std::unique_ptr<BackendRegistry>>(std::make_unique<BackendRegistry>());
    registrar.getLibrary().provide(aylo::contract::platform::kPlatformRegistryService, (*backends)->platforms);
    registrar.getLibrary().provide(aylo::contract::renderer::kRendererRegistryService, (*backends)->renderers);
    registrar.getLibrary().provideNames(aylo::contract::platform::kPlatformRegistryService, [backends] {
        return *backends ? (*backends)->platforms.getNames() : std::vector<std::string>{};
    });
    registrar.getLibrary().provideNames(aylo::contract::renderer::kRendererRegistryService, [backends] {
        return *backends ? (*backends)->renderers.getNames() : std::vector<std::string>{};
    });
    registrar.getPlugins().add(std::string{GraphicsPlugin::kName},
                               [backends](const aylo::core::config::Settings& settings) {
                                   return std::make_unique<GraphicsPlugin>(std::move(*backends), settings);
                               });
}
