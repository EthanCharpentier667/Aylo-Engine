/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GameNameTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "core/layout/GameName.hpp"

using aylo::core::layout::gameName;
using aylo::core::layout::isFileName;

TEST(GameName, IsTheFolderAboveAGameFolder) {
    EXPECT_EQ(gameName(std::filesystem::path{"games"} / "MyGame" / "game"), "MyGame");
    EXPECT_EQ(gameName(std::filesystem::path{"MyGame"} / "game" / ""), "MyGame");  // a trailing separator
}

TEST(GameName, IsTheFolderItselfOtherwise) {
    EXPECT_EQ(gameName(std::filesystem::path{"games"} / "duel"), "duel");
    EXPECT_EQ(gameName(std::filesystem::path{"game"}), "game");  // nothing above it
}

TEST(IsFileName, AcceptsAPlainName) {
    EXPECT_TRUE(isFileName("MyGame"));
    EXPECT_TRUE(isFileName("My Game 2"));
}

TEST(IsFileName, RefusesWhatCannotNameAFolderEverywhere) {
    for (const std::string name : {"", ".", "..", "a/b", "a\\b", "a:b", "a*b", "a?b", "a\"b", "a<b", "a>b", "a|b"}) {
        EXPECT_FALSE(isFileName(name)) << name;
    }
    EXPECT_FALSE(isFileName(std::string{"a\nb"}));
}
