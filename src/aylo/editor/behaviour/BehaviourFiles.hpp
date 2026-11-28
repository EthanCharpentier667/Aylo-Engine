/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BehaviourFiles
*/

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "editor/Export.hpp"

/// @brief What the editor knows of the Behaviours of a game (`engine.behaviour`, `manual/pages/behaviours.md`): their
/// files in `scripts/behaviours/`, and which components are one. A Behaviour is a component and generated systems, so
/// the editor reads them from the App and the files, never from the scripts' runtime.
namespace aylo::editor::behaviour {
/// @return The folder whose files the engine runs without a `require`: `scripts/behaviours/` of the game whose
/// assets are @p assets.
[[nodiscard]] AYLO_EDITOR_API std::filesystem::path behavioursFolder(const std::filesystem::path& assets);

/// @return The source of a new Behaviour `game.<name>`, on the server: a declaration with one field, `onStart` and
/// `onFixedUpdate`, and the other hooks in comments.
[[nodiscard]] AYLO_EDITOR_API std::string makeTemplate(std::string_view name);

/// @brief Writes `<folder>/<name>.luau` from makeTemplate(), creating the folder.
/// @param name What the user typed: a Luau name (`Spinner`); a `.luau` at its end is dropped.
/// @param file Set to the file written.
/// @return Why it was not written (a bad name, a file that exists, a write that failed); empty when it was.
[[nodiscard]] AYLO_EDITOR_API std::string createBehaviour(const std::filesystem::path& folder, std::string_view name,
                                                          std::filesystem::path& file);

/// @return The file of @p folder (at any depth) that declares the Behaviour @p component, the first in path order;
/// none when no file does.
[[nodiscard]] AYLO_EDITOR_API std::optional<std::filesystem::path> findBehaviourFile(
    const std::filesystem::path& folder, std::string_view component);

/// @return Whether the component @p component is a Behaviour with a hook: the App has a system named after it and a
/// hook (`<component>.onUpdate`...) in the hook's phase.
[[nodiscard]] AYLO_EDITOR_API bool isBehaviour(const ecs::App& app, std::string_view component);

/// @return Whether @p component is a tag a Behaviour keeps its lifecycle in (`<behaviour>.Started`,
/// `<behaviour>.Active`): a component the editor does not show.
[[nodiscard]] AYLO_EDITOR_API bool isBehaviourTag(const ecs::ComponentRegistry& registry, std::string_view component);
}  // namespace aylo::editor::behaviour
