/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AssetLoadersTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

#include "aylo/ecs/world/World.hpp"
#include "plugins/AssetLoaders.hpp"

TEST(AssetLoaders, LoadsAPathWithTheLoaderOfItsExtensionWhateverItsCase) {
    aylo::plugins::AssetLoaders loaders;
    std::vector<std::string> models;
    std::vector<std::string> images;
    loaders.add(".obj", [&models](const std::filesystem::path& path) { models.push_back(path.generic_string()); });
    loaders.add(".PNG", [&images](const std::filesystem::path& path) { images.push_back(path.generic_string()); });

    EXPECT_TRUE(loaders.load("models/statue.OBJ"));
    EXPECT_TRUE(loaders.load("sprites/ship.png"));
    EXPECT_EQ(models, (std::vector<std::string>{"models/statue.OBJ"}));
    EXPECT_EQ(images, (std::vector<std::string>{"sprites/ship.png"}));
}

TEST(AssetLoaders, APathNoPluginLoadsIsSkipped) {
    const aylo::plugins::AssetLoaders loaders;
    EXPECT_FALSE(loaders.load("sounds/boom.wav"));
    EXPECT_FALSE(loaders.load("noextension"));
}

TEST(AssetLoaders, SeveralPluginsFillTheSameResource) {
    aylo::ecs::World world;
    int loads = 0;
    aylo::plugins::addAssetLoader(world.getResources(), {".obj", ".glb"},
                                  [&loads](const std::filesystem::path& /*path*/) { ++loads; });
    aylo::plugins::addAssetLoader(world.getResources(), {".material"},
                                  [&loads](const std::filesystem::path& /*path*/) { ++loads; });
    const auto* loaders = world.getResources().get<aylo::plugins::AssetLoaders>();
    ASSERT_NE(loaders, nullptr);
    EXPECT_TRUE(loaders->load("models/a.glb"));
    EXPECT_TRUE(loaders->load("materials/a.material"));
    EXPECT_EQ(loads, 2);
}
