/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include "aylo/contract/renderer/RendererRegistry.hpp"
#include "aylo/render/vulkan/Registration.hpp"
#include "core/library/LibraryEntry.hpp"

AYLO_LIBRARY(registrar) {
    if (auto* registry = registrar.find<aylo::contract::renderer::RendererRegistry>(
            aylo::contract::renderer::kRendererRegistryService)) {
        aylo::render::vulkan::registerRenderer(*registry);
    }
}
