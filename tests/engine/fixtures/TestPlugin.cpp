/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TestPlugin
*/

// A plugin library for the PluginLoader's tests: what a real one is, built on its own.

#include <memory>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/Plugin.hpp"
#include "aylo/ecs/app/PluginEntry.hpp"
#include "core/config/Settings.hpp"

namespace {
/// @brief Records in a resource-free way that it was built: the App's plugin list.
class LoadedPlugin final : public aylo::ecs::Plugin {
  public:
    LoadedPlugin() = default;
    ~LoadedPlugin() override = default;
    LoadedPlugin(const LoadedPlugin& other) = default;
    LoadedPlugin& operator=(const LoadedPlugin& other) = default;
    LoadedPlugin(LoadedPlugin&& other) noexcept = default;
    LoadedPlugin& operator=(LoadedPlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return "test.Loaded"; }
    void build(aylo::ecs::App& /*app*/) override {}
};
}  // namespace

AYLO_PLUGIN_LIBRARY(registrar) {
    registrar.getPlugins().add("test.Loaded", [] { return std::make_unique<LoadedPlugin>(); });
}
