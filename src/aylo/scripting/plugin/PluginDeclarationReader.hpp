/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PluginDeclarationReader
*/

#pragma once

#include <string>

#include "PluginDeclaration.hpp"
#include "aylo/luau/Table.hpp"
#include "aylo/scripting/Export.hpp"
#include "aylo/scripting/Result.hpp"

namespace aylo::scripting::plugin {
/// @brief Reads the table a script gives to `engine.ecs.plugin` as a PluginDeclaration.
///
/// @details The table may have a `build` function and a `plugins` list of plugin names. Nothing is guessed: another
/// key, a `build` that is not a function or a `plugins` with anything but strings is an error. Whether the listed
/// plugins exist is checked when the plugin is added, since they may be declared later.
/// @param name Name of the plugin, "game.Weapons"; it must not be empty.
/// @param spec The declaration table.
/// @return The declaration, or an ErrorKind::kInvalidDeclaration error naming the plugin and what is wrong.
[[nodiscard]] AYLO_SCRIPTING_API Result<PluginDeclaration> readPluginDeclaration(const std::string& name,
                                                                                 const luau::Table& spec);
}  // namespace aylo::scripting::plugin
