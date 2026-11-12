/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ExecutablePathTests
*/

#include <gtest/gtest.h>

#include <filesystem>

#include "core/system/ExecutablePath.hpp"

TEST(ExecutablePath, IsThisTestBinary) {
    const std::filesystem::path path = aylo::core::system::getExecutablePath();
    EXPECT_TRUE(path.is_absolute());
    EXPECT_TRUE(std::filesystem::is_regular_file(path));
    EXPECT_EQ(path.stem(), "vulkan-tests");  // .exe on Windows.
}

TEST(ExecutablePath, DirectoryContainsIt) {
    const std::filesystem::path directory = aylo::core::system::getExecutableDirectory();
    EXPECT_TRUE(std::filesystem::is_directory(directory));
    EXPECT_EQ(directory, aylo::core::system::getExecutablePath().parent_path());
}
