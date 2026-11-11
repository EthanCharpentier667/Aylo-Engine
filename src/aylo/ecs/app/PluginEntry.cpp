/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PluginEntry
*/

#include "PluginEntry.hpp"

#include "aylo/ecs/app/PluginRegistry.hpp"
#include "core/library/LibraryEntry.hpp"

namespace aylo::ecs {
PluginRegistrar::PluginRegistrar(core::library::LibraryRegistrar& library) noexcept : _library{&library} {}

PluginRegistry& PluginRegistrar::getPlugins() const noexcept {
    return *_library->find<PluginRegistry>(kPluginRegistryService);
}

core::library::LibraryRegistrar& PluginRegistrar::getLibrary() const noexcept { return *_library; }
}  // namespace aylo::ecs
