/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PluginDeclaration
*/

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "aylo/luau/FunctionRef.hpp"

namespace aylo::scripting::plugin {
/// @brief A Luau plugin as a script declares it with `engine.ecs.plugin`.
struct PluginDeclaration {
    std::string name;                        ///< Name of the plugin, unique among Luau plugins: "game.Weapons".
    std::vector<std::string> plugins;        ///< Names of the plugins added before this one, in the order written.
    std::optional<luau::FunctionRef> build;  ///< The `build(app)` function; a plugin may only group others.
};
}  // namespace aylo::scripting::plugin
