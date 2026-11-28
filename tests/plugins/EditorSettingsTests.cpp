/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorSettingsTests
*/

#include <gtest/gtest.h>

#include <string>
#include <vector>

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
    settings.set("window.title", std::string{"AYLO Duel"});

    EXPECT_EQ(aylo::editor::readEditorOptions(settings).gameTitle, "AYLO Duel");
    EXPECT_TRUE(aylo::editor::readEditorOptions(Settings{}).gameTitle.empty());
}

TEST(EditorSettings, RefusesAnUnknownKey) {
    Settings settings;
    settings.set("editor.sceen", std::string{"level"});

    EXPECT_ANY_THROW(static_cast<void>(aylo::editor::readEditorOptions(settings)));
}

TEST(EditorSettings, ReadsTheEntryPointsOfTheScripts) {
    // The Assets panel marks the entry points among the scripts (#180): main by default, server and client if named.
    EXPECT_EQ(aylo::editor::readEditorOptions(Settings{}).scriptEntries, std::vector<std::string>{"main"});
    Settings settings;
    settings.set("scripts.main", std::string{"shared/main"});
    settings.set("scripts.server", std::string{"server/main"});
    settings.set("scripts.client", std::string{"client/main"});

    EXPECT_EQ(aylo::editor::readEditorOptions(settings).scriptEntries,
              (std::vector<std::string>{"shared/main", "server/main", "client/main"}));
}
