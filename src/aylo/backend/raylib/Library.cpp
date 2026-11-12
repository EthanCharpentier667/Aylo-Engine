/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include "aylo/contract/platform/PlatformRegistry.hpp"
#include "aylo/contract/renderer/RendererRegistry.hpp"
#include "backend/raylib/Registration.hpp"
#include "core/library/LibraryEntry.hpp"

AYLO_LIBRARY(registrar) {
    if (auto* platforms = registrar.find<aylo::contract::platform::PlatformRegistry>(
            aylo::contract::platform::kPlatformRegistryService)) {
        aylo::backend::raylib::registerPlatform(*platforms);
    }
    if (auto* renderers = registrar.find<aylo::contract::renderer::RendererRegistry>(
            aylo::contract::renderer::kRendererRegistryService)) {
        aylo::backend::raylib::registerRenderer(*renderers);
    }
}
