/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** JsonPluginTests
*/

#include <gtest/gtest.h>

#include "aylo/ecs/app/App.hpp"
#include "plugins/json/JsonPlugin.hpp"

TEST(JsonPlugin, IsNamedAndRequiresNothing) {
    const aylo::plugins::json::JsonPlugin plugin;
    EXPECT_EQ(plugin.getName(), "aylo.Json");
    EXPECT_TRUE(plugin.getDependencies().empty());
}

TEST(JsonPlugin, AppAcceptsItFirst) {
    aylo::ecs::App app;
    EXPECT_NO_THROW(app.addPlugin(aylo::plugins::json::JsonPlugin{}));
}
