/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorSettings
*/

#include "editor/EditorSettings.hpp"

#include "core/config/Settings.hpp"
#include "editor/EditorState.hpp"

namespace aylo::editor {
EditorOptions readEditorOptions(const core::config::Settings& settings) {
    const core::config::Settings editor = settings.section("editor");

    editor.checkKeys({"scene", "textEditor"}, "editor");
    return EditorOptions{
        .scene = editor.getString("scene", ""),
        .textEditor = editor.getString("textEditor", ""),
        .gameTitle = settings.section("window").getString("title", ""),
    };
}
}  // namespace aylo::editor
