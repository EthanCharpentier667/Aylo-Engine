/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DeclarationReader
*/

#pragma once

#include <string>

#include "ComponentDeclaration.hpp"
#include "aylo/luau/Table.hpp"
#include "aylo/scripting/Export.hpp"
#include "aylo/scripting/Result.hpp"
#include "aylo/scripting/asset/NamedAssets.hpp"

namespace aylo::scripting::component {
/// @brief Reads the table a script gives to `engine.ecs.component` as a ComponentDeclaration.
///
/// @details The table has a `fields` table of field names and type names (`bool`, `i8`..`i64`, `u8`..`u64`, `f32`,
/// `f64`, `entity`), and optionally `defaults`, field names and values written like a field is, and `replication`
/// (`"none"`, `"everyChange"` or `"spawnOnly"`) and `presence` (`"both"`, `"serverOnly"` or `"clientOnly"`, `"both"`
/// by default). Nothing is guessed: another key, an unknown or not yet supported field
/// type, a default for a field that does not exist or that its type refuses is an error. Fields come out sorted by
/// name, since a Luau table has no order.
/// @param name Name of the component, "game.Shield".
/// @param spec The declaration table.
/// @param named Where the asset paths of the defaults are recorded (view::writeField()); null: nowhere.
/// @return The declaration, or an ErrorKind::kInvalidDeclaration error naming the component and what is wrong, or an
/// ErrorKind::kInvalidValue one when a default does not fit its field.
[[nodiscard]] AYLO_SCRIPTING_API Result<ComponentDeclaration> readComponentDeclaration(
    std::string name, const luau::Table& spec, asset::NamedAssets* named = nullptr);

/// @brief Reads the table a script gives to `engine.ecs.event` as a declaration: a `fields` table, as a component's,
/// and nothing else. An event is local to one machine (both sides have its type, none sends it to the other), so it
/// has no `replication` or `presence`; it is sent with its fields at zero but those the sender writes, so it has no
/// `defaults` either.
/// @param name Name of the event, "game.Hit".
/// @return The declaration, or an ErrorKind::kInvalidDeclaration error naming the event and what is wrong.
[[nodiscard]] AYLO_SCRIPTING_API Result<ComponentDeclaration> readEventDeclaration(std::string name,
                                                                                   const luau::Table& spec);
}  // namespace aylo::scripting::component
