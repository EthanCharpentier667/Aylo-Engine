/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** NamedAssetsTests
*/

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "aylo/scripting/asset/NamedAssets.hpp"

TEST(NamedAssets, GivesEachPathOnceInTheOrderItWasNamed) {
    aylo::scripting::asset::NamedAssets named;
    named.name("ships/left.png");
    named.name("laser.mp3");
    named.name("ships/left.png");

    EXPECT_EQ(named.take(), (std::vector<std::string>{"ships/left.png", "laser.mp3"}));
    EXPECT_TRUE(named.take().empty());

    named.name("laser.mp3");
    named.name("fonts/orbitron.json");
    EXPECT_EQ(named.take(), std::vector<std::string>{"fonts/orbitron.json"})
        << "a path taken before is not given again";
}
