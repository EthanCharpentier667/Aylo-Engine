/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PluginGroup
*/

#pragma once

#include "aylo/ecs/Export.hpp"

namespace aylo::ecs {
class App;

/// @brief A set of plugins added together (MinimalPlugins, and later DefaultPlugins).
/// @details build() adds each plugin through the App, so a plugin already added is skipped as usual.
/// @code
/// class GamePlugins : public aylo::ecs::PluginGroup {
///   public:
///     void build(aylo::ecs::App& app) override { app.addPlugin(WeaponsPlugin{}).addPlugin(EnemiesPlugin{}); }
/// };
/// app.addPlugins(GamePlugins{});
/// @endcode
class AYLO_ECS_API PluginGroup {
  public:
    virtual ~PluginGroup();

    /// @brief Adds the group's plugins to the App.
    virtual void build(App& app) = 0;

  protected:
    PluginGroup() = default;
    PluginGroup(const PluginGroup& other) = default;
    PluginGroup& operator=(const PluginGroup& other) = default;
    PluginGroup(PluginGroup&& other) noexcept = default;
    PluginGroup& operator=(PluginGroup&& other) noexcept = default;
};
}  // namespace aylo::ecs
