/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RequiredPlugin
*/

#pragma once

#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

#include "aylo/ecs/app/Plugin.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/engine/Export.hpp"

namespace aylo::engine::plugin {
/// @brief Builds a plugin every game needs from what the loaded libraries registered, as the plugins a game lists are:
/// the Engine links none of them.
/// @param registry The plugins the libraries registered.
/// @param name The plugin's name, such as "rtype.Transform".
/// @param searched The folders the libraries were looked for in, named in the error.
/// @return The plugin, never null.
/// @throws exceptions::RuntimeError When no library registered @p name: the message names it, the folders searched
/// and the plugins that exist.
[[nodiscard]] AYLO_ENGINE_API std::unique_ptr<ecs::Plugin> createRequiredPlugin(
    const ecs::PluginRegistry& registry, std::string_view name, const std::vector<std::filesystem::path>& searched);
}  // namespace aylo::engine::plugin
