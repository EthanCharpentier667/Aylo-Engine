/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include "aylo/contract/platform/PlatformRegistry.hpp"
#include "aylo/platform/glfw/Registration.hpp"
#include "core/library/LibraryEntry.hpp"

AYLO_LIBRARY(registrar) {
    if (auto* registry = registrar.find<aylo::contract::platform::PlatformRegistry>(
            aylo::contract::platform::kPlatformRegistryService)) {
        aylo::platform::glfw::registerPlatform(*registry);
    }
}
