/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/config/Settings.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/TextureCalls.hpp"
#include "rtype/contract/platform/PlatformRegistry.hpp"
#include "rtype/contract/renderer/RendererRegistry.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    rtype::plugins::graphics::addTextureForms(registrar.getPlugins());
    using rtype::plugins::graphics::BackendRegistry;
    using rtype::plugins::graphics::GraphicsPlugin;
    auto backends = std::make_shared<std::unique_ptr<BackendRegistry>>(std::make_unique<BackendRegistry>());
    registrar.getLibrary().provide(rtype::contract::platform::kPlatformRegistryService, (*backends)->platforms);
    registrar.getLibrary().provide(rtype::contract::renderer::kRendererRegistryService, (*backends)->renderers);
    registrar.getLibrary().provideNames(rtype::contract::platform::kPlatformRegistryService, [backends] {
        return *backends ? (*backends)->platforms.getNames() : std::vector<std::string>{};
    });
    registrar.getLibrary().provideNames(rtype::contract::renderer::kRendererRegistryService, [backends] {
        return *backends ? (*backends)->renderers.getNames() : std::vector<std::string>{};
    });
    registrar.getPlugins().add(std::string{GraphicsPlugin::kName},
                               [backends](const rtype::core::config::Settings& settings) {
                                   return std::make_unique<GraphicsPlugin>(std::move(*backends), settings);
                               });
}
