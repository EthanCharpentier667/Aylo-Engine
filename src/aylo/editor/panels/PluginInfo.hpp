/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PluginInfo
*/

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "editor/Export.hpp"

namespace aylo::ecs {
class App;
}  // namespace aylo::ecs

namespace aylo::editor::panels {

/// @brief What the Plugins panel shows of a plugin added to the App.
struct PluginRow {
    std::string name;                     ///< Its name (Plugin::getName()).
    std::vector<std::string> requires_;   ///< The plugins it requires (Plugin::getDependencies()).
    std::vector<std::string> requiredBy;  ///< The plugins added that require it, in the order added.
    bool isEngine{false};                 ///< Whether it is an engine plugin (its name starts with "rtype.").
};

/// @brief The plugins of @p app that the Plugins panel lists.
struct PluginList {
    std::vector<PluginRow> loaded;       ///< The plugins added, in the order recorded, matching the filter.
    std::vector<std::string> notLoaded;  ///< The registered plugins that were not added, sorted, matching the filter.
};

/// @return The plugins of @p app, loaded or only available, keeping those whose name contains @p filter (case
/// ignored) when it is not empty. `requiredBy` is computed over every plugin, whatever the filter.
[[nodiscard]] AYLO_EDITOR_API PluginList collectPlugins(const ecs::App& app, std::string_view filter);

}  // namespace aylo::editor::panels
