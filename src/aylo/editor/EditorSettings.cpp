/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorSettings
*/

#include "editor/EditorSettings.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/config/Settings.hpp"
#include "editor/EditorState.hpp"

namespace aylo::editor {
EditorOptions readEditorOptions(const core::config::Settings& settings) {
    const core::config::Settings editor = settings.section("editor");

    editor.checkKeys({"scene", "textEditor"}, "editor");
    // The entries the engine requires (engine/design.md): main by default, server and client when named.
    const core::config::Settings scripts = settings.section("scripts");
    std::vector<std::string> entries{scripts.getString("main", "main")};
    for (const std::string_view side : {"server", "client"}) {
        if (std::string entry = scripts.getString(side, ""); !entry.empty()) {
            entries.push_back(std::move(entry));
        }
    }
    return EditorOptions{
        .scene = editor.getString("scene", ""),
        .textEditor = editor.getString("textEditor", ""),
        .gameTitle = settings.section("window").getString("title", ""),
        .scriptEntries = std::move(entries),
    };
}
}  // namespace aylo::editor
