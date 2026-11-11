/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GameExportTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/engine/Engine.hpp"
#include "aylo/engine/Exceptions.hpp"
#include "aylo/engine/exporting/GameExport.hpp"
#include "aylo/engine/plugin/SharedLibrary.hpp"
#include "aylo/luau/Runtime.hpp"
#include "core/system/ExecutablePath.hpp"
#include "core/vfs/Pack.hpp"

using aylo::engine::Engine;
using aylo::engine::exceptions::RuntimeError;
using aylo::engine::exporting::exportGame;
using aylo::engine::exporting::ExportReport;
using aylo::engine::exporting::ExportRequest;
using aylo::engine::plugin::SharedLibrary;

namespace {
/// @return The file name of the library @p basename as the build names it.
std::string libraryFile(const std::string_view basename) {
#ifdef _WIN32
    return std::format("{}{}", basename, SharedLibrary::getExtension());
#else
    return std::format("lib{}{}", basename, SharedLibrary::getExtension());
#endif
}

/// @return The names of the files directly in @p folder, sorted; none if it does not exist.
std::vector<std::string> filesIn(const std::filesystem::path& folder) {
    std::vector<std::string> names;
    std::error_code error;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator{folder, error}) {
        if (entry.is_regular_file()) {
            names.push_back(entry.path().filename().string());
        }
    }
    std::ranges::sort(names);
    return names;
}

/// @return The text of @p file.
std::string readText(const std::filesystem::path& file) {
    std::ifstream in{file};
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}

/// @brief A game in `<root>/MyGame/game` and an output folder, removed when the test ends; the runtime is the build
/// folder of the tests.
class GameExportTest : public ::testing::Test {
  protected:
    void SetUp() override {
        std::filesystem::create_directories(_game / "scripts");
        write("project.luau", R"(engine.settings.plugins = { "rtype.Physics" })");
        write("scripts/main.luau", "return {}");
    }

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_root, error);
    }

    /// @brief Writes @p text to @p relative, in the game.
    void write(const std::string& relative, const std::string_view text) const {
        std::filesystem::create_directories((_game / relative).parent_path());
        std::ofstream{_game / relative} << text;
    }

    /// @return What exporting the game to the output folder asks, with the name @p name.
    [[nodiscard]] ExportRequest request(std::string name = "") const {
        return {.game = _game,
                .runtime = aylo::core::system::getExecutableDirectory(),
                .output = _output,
                .name = std::move(name),
                .engineVersion = "9.9.9"};
    }

    std::filesystem::path _root{std::filesystem::temp_directory_path() /
                                std::format("aylo_game_export_{:08x}", std::random_device{}())};
    std::filesystem::path _game{_root / "MyGame" / "game"};
    std::filesystem::path _output{_root / "out"};
};
}  // namespace

TEST_F(GameExportTest, ExportsTheProgramTheGameAndOnlyThePluginsItNeeds) {
    const ExportReport report = exportGame(request());
    const std::filesystem::path folder = _output / "MyGame";

    EXPECT_EQ(report.folder, folder);
    // The runtime's program, renamed: "MyGame", or "MyGame.exe" on Windows.
    const std::string program =
        "MyGame" + std::filesystem::path{aylo::engine::exporting::runtimeProgramName()}.extension().string();
    EXPECT_TRUE(std::filesystem::is_regular_file(folder / program));
    EXPECT_EQ(filesIn(folder / Engine::kPluginsDirectory),
              (std::vector<std::string>{libraryFile("aylo-plugin-physics"), libraryFile("aylo-plugin-transform")}));
    EXPECT_TRUE(filesIn(folder / Engine::kPluginsDirectory / Engine::kBackendsDirectory).empty());
    EXPECT_TRUE(std::filesystem::is_regular_file(folder / Engine::kPackFile));
    EXPECT_FALSE(std::filesystem::exists(folder / "game" / Engine::kProjectFile));  // in the pack
    EXPECT_TRUE(std::filesystem::is_regular_file(folder / "licenses" / "THIRD_PARTY_NOTICES.md"));
    EXPECT_GT(report.bytes, 0U);
}

TEST_F(GameExportTest, WritesTheLibrariesAndTheHashesInExportJson) {
    static_cast<void>(exportGame(request()));
    const std::string json = readText(_output / "MyGame" / "export.json");

    EXPECT_NE(json.find(R"("engine": "9.9.9")"), std::string::npos) << json;
    EXPECT_NE(json.find(std::format(R"("file": "plugins/{}")", libraryFile("aylo-plugin-physics"))), std::string::npos)
        << json;
    EXPECT_NE(json.find(R"json("named in plugins (server)")json"), std::string::npos) << json;
    EXPECT_NE(json.find(R"("game.pak": ")"), std::string::npos) << json;
}

TEST_F(GameExportTest, PacksTheGameWithItsScriptsPrecompiled) {
    write("assets/levels/a.txt", "a level");
    static_cast<void>(exportGame(request()));
    const aylo::core::vfs::PackSource pack{_output / "MyGame" / Engine::kPackFile};

    EXPECT_EQ(pack.read("project.luau"), R"(engine.settings.plugins = { "rtype.Physics" })");
    EXPECT_EQ(pack.read("assets/levels/a.txt"), "a level");
    EXPECT_TRUE(std::string_view{pack.read("scripts/main.luau")}.starts_with(aylo::luau::Runtime::kBytecodeHeader));
    EXPECT_EQ(readText(_output / "MyGame" / "play.sh").find("game.pak") == std::string::npos,
              aylo::engine::exporting::runtimeProgramName().ends_with(".exe"));  // play.bat on Windows
}

TEST_F(GameExportTest, TheEngineRunsTheExportedPack) {
    write("project.luau", "engine.settings.network.port = 0");
    write("scripts/shared/names.luau", "return { shield = 'game.Shield' }");
    write("scripts/main.luau", R"(engine.ecs.component(require("@scripts/shared/names").shield, { fields = {} }))");
    static_cast<void>(exportGame(request()));

    const Engine engine{
        {.mode = aylo::ecs::RunMode::kServer, .args = {(_output / "MyGame" / Engine::kPackFile).string()}}};

    EXPECT_TRUE(engine.isPacked());
    EXPECT_TRUE(engine.app().getWorld().getComponents().find("game.Shield"));
}

TEST_F(GameExportTest, RefusesAScriptThatDoesNotCompile) {
    write("scripts/broken.luau", "local = ");

    try {
        static_cast<void>(exportGame(request()));
        FAIL() << "a script that does not compile was exported";
    } catch (const RuntimeError& error) {
        EXPECT_NE(std::string_view{error.what()}.find("scripts/broken.luau"), std::string_view::npos) << error.what();
    }
    EXPECT_FALSE(std::filesystem::exists(_output / "MyGame"));
}

TEST_F(GameExportTest, KeepsTheGameFolderWithoutAPack) {
    write(".luaurc", "{}");
    write("plugins/boss/Boss.cpp", "// a source");
    write("plugins/boss/xmake.lua", "-- a build file");
    ExportRequest folder = request();
    folder.pack = false;
    static_cast<void>(exportGame(folder));
    const std::filesystem::path game = _output / "MyGame" / "game";

    EXPECT_FALSE(std::filesystem::exists(_output / "MyGame" / Engine::kPackFile));
    EXPECT_EQ(readText(game / Engine::kProjectFile), R"(engine.settings.plugins = { "rtype.Physics" })");
    EXPECT_EQ(readText(game / "scripts" / "main.luau"), "return {}");
    EXPECT_FALSE(std::filesystem::exists(game / ".luaurc"));
    EXPECT_FALSE(std::filesystem::exists(game / "plugins" / "boss"));
}

TEST_F(GameExportTest, KeepsTheLibrariesOfTheGamePluginsNextToThePack) {
    // The fixture under another name: two libraries of the same file name are refused.
    const std::string library = libraryFile("aylo-game-boss");
    std::filesystem::create_directories(_game / "plugins");
    std::filesystem::copy_file(
        aylo::core::system::getExecutableDirectory() / "plugins" / libraryFile("aylo-test-plugin"),
        _game / "plugins" / library);
    write("plugins/" + library + ".sig", "a signature");
    write("plugins/boss/Boss.cpp", "// a source");
    // A release build checks the signature of a game's plugin before it opens it; the fixture has none.
    ExportRequest unchecked = request();
    unchecked.verifyGamePlugin = {};
    static_cast<void>(exportGame(unchecked));
    const std::filesystem::path plugins = _output / "MyGame" / "game" / "plugins";

    EXPECT_TRUE(std::filesystem::is_regular_file(plugins / library));
    EXPECT_EQ(readText(plugins / (library + ".sig")), "a signature");
    EXPECT_FALSE(std::filesystem::exists(plugins / "boss"));
    EXPECT_FALSE(aylo::core::vfs::PackSource{_output / "MyGame" / Engine::kPackFile}.exists("plugins/" + library));
}

TEST_F(GameExportTest, WarnsWhenTheRuntimeHasNoNotices) {
    // The build folder has no licenses/by-library/: only the release zip has them.
    const ExportReport report = exportGame(request());

    ASSERT_FALSE(report.warnings.empty());
    EXPECT_NE(report.warnings.front().find("notices"), std::string::npos) << report.warnings.front();
}

TEST_F(GameExportTest, NamesTheExportAsAsked) {
    const ExportReport report = exportGame(request("Duel"));

    EXPECT_EQ(report.folder, _output / "Duel");
    EXPECT_TRUE(std::filesystem::exists(_output / "Duel" / Engine::kPackFile));
}

TEST_F(GameExportTest, RefusesANameThatIsNotAFileName) {
    EXPECT_THROW(static_cast<void>(exportGame(request("a/b"))), RuntimeError);
    EXPECT_FALSE(std::filesystem::exists(_output));
}

TEST_F(GameExportTest, RefusesAnOutputFolderThatIsNotEmpty) {
    std::filesystem::create_directories(_output / "MyGame");
    std::ofstream{_output / "MyGame" / "keep.txt"} << "mine";

    EXPECT_THROW(static_cast<void>(exportGame(request())), RuntimeError);
    EXPECT_EQ(readText(_output / "MyGame" / "keep.txt"), "mine");
}

TEST_F(GameExportTest, LeavesNothingWhenTheGameIsRefused) {
    write("project.luau", R"(engine.settings.plugins = { "game.Missing" })");

    EXPECT_THROW(static_cast<void>(exportGame(request())), RuntimeError);
    EXPECT_FALSE(std::filesystem::exists(_output / "MyGame"));
}
