/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RequiredPluginTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>

#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/engine/Exceptions.hpp"
#include "aylo/engine/plugin/RequiredPlugin.hpp"
#include "aylo/plugins/transform/TransformPlugin.hpp"

TEST(RequiredPlugin, FailsClearlyWhenNoLibraryProvidesIt) {
    const aylo::ecs::PluginRegistry registry;

    try {
        [[maybe_unused]] const auto plugin = aylo::engine::plugin::createRequiredPlugin(
            registry, aylo::plugins::transform::TransformPlugin::kName, {std::filesystem::path{"some/plugins"}});
        FAIL() << "expected an error";
    } catch (const aylo::engine::exceptions::RuntimeError& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find(std::string{aylo::plugins::transform::TransformPlugin::kName}), std::string::npos);
        EXPECT_NE(message.find("some/plugins"), std::string::npos);
    }
}

TEST(RequiredPlugin, BuildsThePluginAnyLibraryRegistered) {
    aylo::ecs::PluginRegistry registry;
    registry.add(std::string{aylo::plugins::transform::TransformPlugin::kName},
                 [] { return std::make_unique<aylo::plugins::transform::TransformPlugin>(); });

    const auto plugin =
        aylo::engine::plugin::createRequiredPlugin(registry, aylo::plugins::transform::TransformPlugin::kName, {});

    EXPECT_NE(plugin, nullptr);
}
