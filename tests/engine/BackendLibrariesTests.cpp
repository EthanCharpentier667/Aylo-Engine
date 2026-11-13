/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BackendLibrariesTests
*/

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "aylo/core/config/Settings.hpp"
#include "aylo/engine/Engine.hpp"
#include "aylo/engine/Exceptions.hpp"
#include "aylo/engine/plugin/BackendLibraries.hpp"
#include "aylo/engine/plugin/SharedLibrary.hpp"

namespace {
using aylo::engine::Engine;
using aylo::engine::plugin::backendLibraries;
using aylo::engine::plugin::SharedLibrary;

/// @brief A backends folder of empty files named as the real libraries; only the names are read.
class BackendLibrariesTest : public ::testing::Test {
  protected:
    BackendLibrariesTest() {
        std::filesystem::remove_all(_directory);
        std::filesystem::create_directories(_directory);
        for (const std::string_view name : {"aylo-platform-glfw", "aylo-platform-sdl", "aylo-render-vulkan",
                                            "aylo-audio-miniaudio", "aylo-backend-raylib", "aylo-backend-null"}) {
            std::ofstream{file(name)};
        }
        std::ofstream{_directory / "notes.txt"};
    }

    ~BackendLibrariesTest() override {
        std::error_code error;
        std::filesystem::remove_all(_directory, error);
    }

    BackendLibrariesTest(const BackendLibrariesTest&) = delete;
    BackendLibrariesTest& operator=(const BackendLibrariesTest&) = delete;
    BackendLibrariesTest(BackendLibrariesTest&&) = delete;
    BackendLibrariesTest& operator=(BackendLibrariesTest&&) = delete;

    /// @return The file of the library @p name, as this system names it.
    [[nodiscard]] std::filesystem::path file(const std::string_view name) const {
        return _directory / (std::string{name} + std::string{SharedLibrary::getExtension()});
    }

    [[nodiscard]] std::vector<std::filesystem::path> select(const aylo::core::config::Settings& settings) const {
        return backendLibraries(_directory, settings, Engine::kBackendSettings);
    }

    std::filesystem::path _directory{std::filesystem::temp_directory_path() / "aylo-backend-libraries"};
};
}  // namespace

TEST_F(BackendLibrariesTest, KeepsOnlyTheBackendsTheSettingsName) {
    aylo::core::config::Settings settings;
    settings.set("platform", std::string{"glfw"});
    settings.set("renderer", std::string{"vulkan"});
    settings.set("audio.backend", std::string{"miniaudio"});

    EXPECT_EQ(select(settings),
              (std::vector{file("aylo-audio-miniaudio"), file("aylo-platform-glfw"), file("aylo-render-vulkan")}));
}

TEST_F(BackendLibrariesTest, NeverOpensRaylibNextToTheGlfwPlatform) {
    aylo::core::config::Settings glfw;
    glfw.set("platform", std::string{"glfw"});
    aylo::core::config::Settings raylib;
    raylib.set("platform", std::string{"raylib"});
    raylib.set("renderer", std::string{"raylib"});

    EXPECT_EQ(select(glfw), (std::vector{file("aylo-platform-glfw")}));
    EXPECT_EQ(select(raylib), (std::vector{file("aylo-backend-raylib")}));  // one library for both settings
}

TEST_F(BackendLibrariesTest, OpensNoneWithoutABackendSetting) {
    EXPECT_TRUE(select(aylo::core::config::Settings{}).empty());
}

TEST_F(BackendLibrariesTest, RefusesABackendNoLibraryProvidesAndListsTheOthers) {
    aylo::core::config::Settings settings;
    settings.set("platform", std::string{"missing"});
    try {
        static_cast<void>(select(settings));
        FAIL() << "an unknown backend was accepted";
    } catch (const aylo::engine::exceptions::RuntimeError& error) {
        const std::string message = error.what();
        EXPECT_TRUE(message.contains("Invalid setting 'platform'")) << message;
        EXPECT_TRUE(message.contains("'missing'")) << message;
        EXPECT_TRUE(message.contains("'glfw'") && message.contains("'raylib'")) << message;
    }
}
