/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EnginePackTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "core/vfs/Pack.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/Engine.hpp"
#include "rtype/engine/EngineConfig.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/plugins/AssetDirectory.hpp"

using rtype::core::vfs::PackEntry;
using rtype::engine::Engine;
using rtype::engine::EngineConfig;
using rtype::engine::exceptions::RuntimeError;

namespace {
/// @brief The project of the packed test games: a server on a port the system picks, and hot reload asked for.
constexpr std::string_view kProject = "engine.settings.network.port = 0\nengine.settings.hotReload = true\n";

/// @brief A folder of its own for the packs of a test, removed when it ends.
class EnginePackTest : public ::testing::Test {
  protected:
    void SetUp() override { std::filesystem::create_directories(_root); }

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_root, error);
    }

    /// @return The pack `<root>/game.pak` made of @p entries.
    [[nodiscard]] std::filesystem::path pack(const std::vector<PackEntry>& entries) const {
        const std::filesystem::path file = _root / "game.pak";
        rtype::core::vfs::writePack(file, entries);
        return file;
    }

    /// @return A headless server config that runs the pack @p file.
    [[nodiscard]] static EngineConfig serverOf(const std::filesystem::path& file) {
        return {.mode = rtype::ecs::RunMode::kServer, .args = {file.string()}};
    }

    std::filesystem::path _root{std::filesystem::temp_directory_path() /
                                std::format("rtype_engine_pack_{:08x}", std::random_device{}())};
};
}  // namespace

TEST_F(EnginePackTest, RunsTheGameOfAPack) {
    const std::filesystem::path file =
        pack({PackEntry{.path = "project.luau", .data = std::string{kProject}},
              PackEntry{.path = "scripts/main.luau",
                        .data = std::string{R"(engine.ecs.component("game.Shield", { fields = {} }))"}}});

    testing::internal::CaptureStderr();
    const Engine engine{serverOf(file)};
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(engine.isPacked());
    EXPECT_TRUE(engine.app().getWorld().getComponents().find("game.Shield"));
    // The native plugins of a packed game are next to the pack, in the folder named after it.
    EXPECT_EQ(engine.gameDirectory(), std::filesystem::absolute(_root / "game").lexically_normal());
    EXPECT_NE(log.find("Hot reload is off: the game is a pack"), std::string::npos) << log;
}

TEST_F(EnginePackTest, TheScriptsOfAPackRequireEachOther) {
    const std::filesystem::path file = pack({
        PackEntry{.path = "project.luau", .data = std::string{kProject}},
        PackEntry{.path = "scripts/shared/config.luau", .data = std::string{"return { name = 'game.Configured' }"}},
        PackEntry{.path = "scripts/main.luau", .data = std::string{R"(
            local config = require("@scripts/shared/config")
            engine.ecs.component(config.name, { fields = {} })
        )"}},
    });

    const Engine engine{serverOf(file)};

    EXPECT_TRUE(engine.app().getWorld().getComponents().find("game.Configured"));
}

TEST_F(EnginePackTest, TheAssetsOfAPackAreReadFromIt) {
    const std::filesystem::path file = pack({PackEntry{.path = "project.luau", .data = std::string{kProject}},
                                             PackEntry{.path = "assets/levels/a.txt", .data = std::string{"in pack"}}});

    const Engine engine{serverOf(file)};
    const auto& resources = engine.app().getWorld().getResources();

    EXPECT_TRUE(rtype::plugins::assetDirectoryOf(resources).empty());
    EXPECT_EQ(rtype::plugins::assetSourceOf(resources)->read("levels/a.txt"), "in pack");
}

TEST_F(EnginePackTest, RefusesAPackWithoutAProject) {
    const std::filesystem::path file = pack({PackEntry{.path = "scripts/main.luau", .data = std::string{"return 1"}}});

    EXPECT_THROW(Engine{serverOf(file)}, RuntimeError);
}

TEST_F(EnginePackTest, RefusesAFileThatIsNotAPack) {
    const std::filesystem::path file = _root / "game.pak";
    std::ofstream{file} << "not a pack at all, only text";

    try {
        const Engine engine{serverOf(file)};
        FAIL() << "a file that is not a pack was run";
    } catch (const RuntimeError& error) {
        EXPECT_NE(std::string_view{error.what()}.find("is not a game pack"), std::string_view::npos) << error.what();
    }
}
