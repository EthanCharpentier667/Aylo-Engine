/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorAssetKindTests
*/

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "editor/panels/AssetKind.hpp"

using aylo::editor::panels::AssetKind;
using aylo::editor::panels::kindOf;

TEST(EditorAssetKind, TellsFilesApartByExtension) {
    EXPECT_EQ(kindOf("level.scene", false), AssetKind::kScene);
    EXPECT_EQ(kindOf("scenes/boss.actor", false), AssetKind::kActor);
    EXPECT_EQ(kindOf("thing.capsule", false), AssetKind::kCapsule);
    EXPECT_EQ(kindOf("main.luau", false), AssetKind::kScript);
    EXPECT_EQ(kindOf("ship.png", false), AssetKind::kImage);
    EXPECT_EQ(kindOf("laser.ogg", false), AssetKind::kAudio);
    EXPECT_EQ(kindOf("ui.ttf", false), AssetKind::kFont);
    EXPECT_EQ(kindOf("map.tmj", false), AssetKind::kData);
    EXPECT_EQ(kindOf("README.md", false), AssetKind::kText);
    EXPECT_EQ(kindOf("shaders/flash.glsl", false), AssetKind::kShader);
    EXPECT_EQ(kindOf("materials/hit.material", false), AssetKind::kMaterial);
    EXPECT_EQ(kindOf("models/crate.glb", false), AssetKind::kModel);
    EXPECT_EQ(kindOf("models/crate.gltf", false), AssetKind::kModel);
    EXPECT_EQ(kindOf("models/crate.fbx", false), AssetKind::kModel);
    EXPECT_EQ(kindOf("models/crate.obj", false), AssetKind::kModel);
}

TEST(EditorAssetKind, ShadersMaterialsAndModelsHaveIconsOfTheirOwn) {
    using aylo::editor::panels::iconOf;
    EXPECT_NE(iconOf(AssetKind::kShader), iconOf(AssetKind::kScript));
    EXPECT_NE(iconOf(AssetKind::kMaterial), iconOf(AssetKind::kImage));
    EXPECT_NE(iconOf(AssetKind::kModel), iconOf(AssetKind::kScene));
    EXPECT_EQ(aylo::editor::panels::toUtf8(iconOf(AssetKind::kModel)).size(), 4U);  // beyond the BMP: four bytes
}

TEST(EditorAssetKind, IgnoresCaseAndFallsBackToOther) {
    EXPECT_EQ(kindOf("SHIP.PNG", false), AssetKind::kImage);
    EXPECT_EQ(kindOf("archive.zip", false), AssetKind::kOther);
    EXPECT_EQ(kindOf("noextension", false), AssetKind::kOther);
    EXPECT_EQ(kindOf(".gitkeep", false), AssetKind::kOther);
}

TEST(EditorAssetKind, AFolderIsAFolderWhateverItsName) { EXPECT_EQ(kindOf("sprites.png", true), AssetKind::kFolder); }

TEST(EditorAssetKind, EveryKindHasAnIconAndALabel) {
    for (int i = 0; i <= static_cast<int>(AssetKind::kOther); ++i) {
        const auto kind = static_cast<AssetKind>(i);
        EXPECT_NE(aylo::editor::panels::iconOf(kind), U'\0');
        EXPECT_FALSE(aylo::editor::panels::labelOf(kind).empty());
    }
    EXPECT_NE(aylo::editor::panels::iconOf(AssetKind::kFolder, true), aylo::editor::panels::iconOf(AssetKind::kFolder));
}

TEST(EditorAssetKind, EncodesCodePointsAsUtf8) {
    EXPECT_EQ(aylo::editor::panels::toUtf8(U'A'), "A");
    EXPECT_EQ(aylo::editor::panels::toUtf8(U'\u00e9'), "\xC3\xA9");
    EXPECT_EQ(aylo::editor::panels::toUtf8(U'\uf07b'), "\xEF\x81\xBB");
    EXPECT_EQ(aylo::editor::panels::toUtf8(U'\U0001F600'), "\xF0\x9F\x98\x80");
}

TEST(EditorAssetKind, TellsTheScriptsApartByWhatTheyAreToTheGame) {
    // In scripts/, a file says where the code that runs comes from (#180): a Behaviour, an entry point, a module.
    using aylo::editor::panels::scriptKindOf;
    const std::vector<std::string> entries{"shared/main", "server/main", "client/main"};
    EXPECT_EQ(scriptKindOf("behaviours/Lifetime.luau", entries, false), AssetKind::kBehaviour);
    EXPECT_EQ(scriptKindOf("behaviours/fx/Spark.luau", entries, false), AssetKind::kBehaviour);
    EXPECT_EQ(scriptKindOf("server/main.luau", entries, false), AssetKind::kEntry);
    EXPECT_EQ(scriptKindOf("shared/main.luau", entries, false), AssetKind::kEntry);
    EXPECT_EQ(scriptKindOf("server/weapons.luau", entries, false), AssetKind::kScript);
    EXPECT_EQ(scriptKindOf("main.luau", entries, false), AssetKind::kScript) << "not named by project.luau here";
    EXPECT_EQ(scriptKindOf("types/aylo.d.luau", entries, false), AssetKind::kTypes);
    EXPECT_EQ(scriptKindOf("behaviours", entries, true), AssetKind::kFolder);
    EXPECT_EQ(scriptKindOf("notes.md", entries, false), AssetKind::kText);

    using aylo::editor::panels::iconOf;
    EXPECT_NE(iconOf(AssetKind::kBehaviour), iconOf(AssetKind::kScript));
    EXPECT_NE(iconOf(AssetKind::kEntry), iconOf(AssetKind::kScript));
    EXPECT_NE(iconOf(AssetKind::kTypes), iconOf(AssetKind::kScript));
}
