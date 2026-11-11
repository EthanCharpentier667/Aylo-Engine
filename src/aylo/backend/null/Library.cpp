/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include <memory>

#include "aylo/contract/platform/IPlatform.hpp"
#include "aylo/contract/platform/PlatformRegistry.hpp"
#include "aylo/contract/renderer/IRenderer.hpp"
#include "aylo/contract/renderer/RendererRegistry.hpp"
#include "backend/null/NullBackend.hpp"
#include "core/config/Settings.hpp"
#include "core/library/LibraryEntry.hpp"

AYLO_LIBRARY(registrar) {
    using namespace aylo;
    if (auto* platforms =
            registrar.find<contract::platform::PlatformRegistry>(contract::platform::kPlatformRegistryService)) {
        platforms->add("null",
                       [](const core::config::Settings& settings) -> std::unique_ptr<contract::platform::IPlatform> {
                           settings.checkKeys({}, "null");
                           return std::make_unique<backend::null::NullPlatform>();
                       });
    }
    if (auto* renderers =
            registrar.find<contract::renderer::RendererRegistry>(contract::renderer::kRendererRegistryService)) {
        renderers->add("null",
                       [](const core::config::Settings& settings) -> std::unique_ptr<contract::renderer::IRenderer> {
                           settings.checkKeys({}, "null");
                           return std::make_unique<backend::null::NullRenderer>();
                       });
    }
}
