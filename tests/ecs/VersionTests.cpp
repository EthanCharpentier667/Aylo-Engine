/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** VersionTests
*/

#include <gtest/gtest.h>

#include <regex>
#include <string>

#include "aylo/ecs/Version.hpp"

TEST(Version, IsNotEmpty) { EXPECT_FALSE(aylo::ecs::getVersion().empty()); }

TEST(Version, IsMajorMinorPatch) {
    const std::string version{aylo::ecs::getVersion()};
    EXPECT_TRUE(std::regex_match(version, std::regex{R"(\d+\.\d+\.\d+)"})) << "version: " << version;
}
