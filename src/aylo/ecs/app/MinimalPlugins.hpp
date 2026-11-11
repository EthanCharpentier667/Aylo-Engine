/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MinimalPlugins
*/

#pragma once

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/app/PluginGroup.hpp"

namespace aylo::ecs {
class App;

/// @brief The plugins every App needs, without a window: a dedicated server, tests.
/// @details For now TimePlugin with the defaults (60 ticks per second, at most 5 per frame); asset metadata and
/// scripting join it when they exist. To configure time, add a TimePlugin before the group: the group's one is then
/// skipped.
/// @code
/// app.addPlugin(aylo::ecs::TimePlugin{aylo::ecs::FixedTimestep{30, 5}})
///     .addPlugins(aylo::ecs::MinimalPlugins{});
/// @endcode
class AYLO_ECS_API MinimalPlugins : public PluginGroup {
  public:
    MinimalPlugins() = default;
    ~MinimalPlugins() override = default;
    MinimalPlugins(const MinimalPlugins& other) = default;
    MinimalPlugins& operator=(const MinimalPlugins& other) = default;
    MinimalPlugins(MinimalPlugins&& other) noexcept = default;
    MinimalPlugins& operator=(MinimalPlugins&& other) noexcept = default;

    /// @brief Adds TimePlugin.
    void build(App& app) override;
};
}  // namespace aylo::ecs
