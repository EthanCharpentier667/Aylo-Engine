/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorSettingsTests
*/

#include <gtest/gtest.h>

#include <string>

#include "aylo/core/config/exceptions/ConfigExceptions.hpp"
#include "core/config/Settings.hpp"
#include "editor/EditorSettings.hpp"

namespace {
using aylo::core::config::Settings;
}  // namespace

TEST(EditorSettings, DefaultsToNoSceneAndTheDefaultTextEditor) {
    const aylo::editor::EditorOptions options = aylo::editor::readEditorOptions(Settings{});

    EXPECT_TRUE(options.scene.empty());
    EXPECT_TRUE(options.textEditor.empty());
}

TEST(EditorSettings, ReadsTheSceneAndTheTextEditor) {
    Settings settings;
    settings.set("editor.scene", std::string{"scenes/level.scene"});
    settings.set("editor.textEditor", std::string{"subl"});

    const aylo::editor::EditorOptions options = aylo::editor::readEditorOptions(settings);

    EXPECT_EQ(options.scene, "scenes/level.scene");
    EXPECT_EQ(options.textEditor, "subl");
}

TEST(EditorSettings, ReadsTheGameWindowTitle) {
    Settings settings;
    settings.set("window.title", std::string{"R-Type Duel"});

    EXPECT_EQ(aylo::editor::readEditorOptions(settings).gameTitle, "R-Type Duel");
    EXPECT_TRUE(aylo::editor::readEditorOptions(Settings{}).gameTitle.empty());
}

TEST(EditorSettings, RefusesAnUnknownKey) {
    Settings settings;
    settings.set("editor.sceen", std::string{"level"});

    EXPECT_ANY_THROW(static_cast<void>(aylo::editor::readEditorOptions(settings)));
}
