/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TypeDefinitions
*/

#pragma once

#include <string>
#include <string_view>

#include "aylo/ecs/call/Calls.hpp"
#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/event/EventRegistry.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/luau/Table.hpp"
#include "aylo/scripting/Export.hpp"

namespace aylo::scripting::types {
/// @brief Where the file of a game's types goes, in its folder: what luau-lsp reads (`luau-lsp.types.definitionFiles`).
inline constexpr std::string_view kDefinitionsFile = "types/aylo.d.luau";

/// @brief What the definitions describe: what the running game knows. None of them is null.
struct TypeSources {
    const ecs::ComponentRegistry* components{
        nullptr};                               ///< Every component, the engine's and the game's, with its fields.
    const ecs::EventRegistry* events{nullptr};  ///< Every event, with its fields.
    const ecs::Calls* calls{nullptr};           ///< Every engine call, with its arguments and result.
    const luau::Table* engine{nullptr};         ///< The `engine` global, walked for its tree of tables and forms.
};

/// @return A luau-lsp definition file (`.d.luau`) for the scripts of the game: the `Entity` and `AssetId` classes, one
/// view type per component and per event, `ctx` (with `ctx:get`, `ctx.call` and `ctx.events` overloaded on the names
/// as string literals), a Behaviour's `self` and its handles, and `engine`: its tree as the game has it, the common
/// forms typed, every other function taking and giving anything.
/// @details Deterministic: the same game gives the same text, so the file is rewritten only when it changes.
[[nodiscard]] AYLO_SCRIPTING_API std::string generateDefinitions(const TypeSources& sources);

/// @return The Luau type a script reads from a field: `number`, `Vec2`, `vector`, `Entity`, a union of an enum's
/// names...
[[nodiscard]] AYLO_SCRIPTING_API std::string readTypeOf(const ecs::FieldInfo& field);

/// @return @p name as a Luau type name: `C_` (component), `E_` (event) or `R_` (call result) and the name with every
/// character that is not a letter, a digit or `_` turned into `_` (`aylo.Transform2D`: `C_aylo_Transform2D`).
[[nodiscard]] AYLO_SCRIPTING_API std::string typeNameOf(std::string_view prefix, std::string_view name);
}  // namespace aylo::scripting::types
