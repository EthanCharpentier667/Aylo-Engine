/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LibrarySelectionTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "core/system/ExecutablePath.hpp"
#include "rtype/engine/Engine.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/engine/exporting/LibrarySelection.hpp"
#include "rtype/engine/plugin/SharedLibrary.hpp"

using rtype::engine::Engine;
using rtype::engine::exceptions::RuntimeError;
using rtype::engine::exporting::LibraryKind;
using rtype::engine::exporting::SelectedLibrary;
using rtype::engine::exporting::selectLibraries;
using rtype::engine::plugin::SharedLibrary;

namespace {
/// @return The file name of the library @p basename as the build names it ("rtype-plugin-shader").
std::string libraryFile(const std::string_view basename) {
#ifdef _WIN32
    return std::format("{}{}", basename, SharedLibrary::getExtension());
#else
    return std::format("lib{}{}", basename, SharedLibrary::getExtension());
#endif
}

/// @return The library of @p selected whose file is @p basename, or null.
const SelectedLibrary* find(const std::vector<SelectedLibrary>& selected, const std::string_view basename) {
    const std::string file = libraryFile(basename);
    const auto found = std::ranges::find_if(
        selected, [&file](const SelectedLibrary& library) { return library.file.filename() == file; });
    return found != selected.end() ? &*found : nullptr;
}

/// @return Whether @p library has the reason @p reason.
bool hasReason(const SelectedLibrary& library, const std::string_view reason) {
    return std::ranges::find(library.reasons, reason) != library.reasons.end();
}

/// @brief A game folder of its own, removed when the test ends; the runtime is the build folder of the tests.
class LibrarySelectionTest : public ::testing::Test {
  protected:
    void SetUp() override { std::filesystem::create_directories(_game); }

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_game, error);
    }

    /// @brief Writes the game's `project.luau`.
    void writeProject(const std::string_view text) const { std::ofstream{_game / Engine::kProjectFile} << text; }

    /// @return The libraries the game needs, from the build folder's.
    [[nodiscard]] std::vector<SelectedLibrary> select() const {
        return selectLibraries({.game = _game, .runtime = rtype::core::system::getExecutableDirectory()});
    }

    std::filesystem::path _game{std::filesystem::temp_directory_path() /
                                std::format("rtype_library_selection_{:08x}", std::random_device{}())};
};
}  // namespace

TEST_F(LibrarySelectionTest, KeepsTheTransformPluginOfEveryGame) {
    writeProject("");
    const std::vector<SelectedLibrary> selected = select();

    ASSERT_EQ(selected.size(), 1U);
    EXPECT_EQ(selected.front().kind, LibraryKind::kPlugin);
    EXPECT_EQ(selected.front().file.filename(), libraryFile("rtype-plugin-transform"));
    EXPECT_TRUE(hasReason(selected.front(), "every game"));
}

TEST_F(LibrarySelectionTest, KeepsTheNamedPluginsAndWhatTheyRequire) {
    writeProject(R"(engine.settings.plugins = { "rtype.Shader" })");
    const std::vector<SelectedLibrary> selected = select();

    const SelectedLibrary* shader = find(selected, "rtype-plugin-shader");
    const SelectedLibrary* graphics = find(selected, "rtype-plugin-graphics");
    ASSERT_NE(shader, nullptr);
    ASSERT_NE(graphics, nullptr);
    EXPECT_TRUE(hasReason(*shader, "named in plugins (client)"));
    EXPECT_TRUE(hasReason(*shader, "named in plugins (server)"));
    EXPECT_TRUE(hasReason(*graphics, "required by rtype.Shader"));
    EXPECT_NE(find(selected, "rtype-plugin-input"), nullptr);  // the graphics plugin requires it
    EXPECT_EQ(find(selected, "rtype-plugin-physics"), nullptr);
    EXPECT_EQ(find(selected, "rtype-plugin-sprite"), nullptr);
    EXPECT_TRUE(std::ranges::none_of(
        selected, [](const SelectedLibrary& library) { return library.kind == LibraryKind::kBackend; }));
}

TEST_F(LibrarySelectionTest, KeepsThePluginsOfEachSide) {
    writeProject(R"(engine.settings.plugins = engine.isServer and { "rtype.Physics" } or {})");
    const std::vector<SelectedLibrary> selected = select();

    const SelectedLibrary* physics = find(selected, "rtype-plugin-physics");
    ASSERT_NE(physics, nullptr);
    EXPECT_TRUE(hasReason(*physics, "named in plugins (server)"));
    EXPECT_FALSE(hasReason(*physics, "named in plugins (client)"));
}

TEST_F(LibrarySelectionTest, KeepsOnlyTheBackendsTheClientNames) {
    writeProject(R"(
        engine.settings.plugins = { "rtype.graphics" }
        if engine.isClient then
            engine.settings.platform = "null"
            engine.settings.renderer = "null"
        end
    )");
    const std::vector<SelectedLibrary> selected = select();

    const SelectedLibrary* null = find(selected, "rtype-backend-null");
    ASSERT_NE(null, nullptr);
    EXPECT_EQ(null->kind, LibraryKind::kBackend);
    EXPECT_TRUE(hasReason(*null, "backend 'null' (platform)"));
    EXPECT_TRUE(hasReason(*null, "backend 'null' (renderer)"));
    const auto backends = std::ranges::count_if(
        selected, [](const SelectedLibrary& library) { return library.kind == LibraryKind::kBackend; });
    EXPECT_EQ(backends, 1);
}

TEST_F(LibrarySelectionTest, RefusesAPluginNoLibraryRegisters) {
    writeProject(R"(engine.settings.plugins = { "game.Missing" })");
    try {
        static_cast<void>(select());
        FAIL() << "an unknown plugin was accepted";
    } catch (const RuntimeError& error) {
        EXPECT_NE(std::string_view{error.what()}.find("game.Missing"), std::string_view::npos) << error.what();
    }
}

TEST_F(LibrarySelectionTest, RefusesABackendNoLibraryRegisters) {
    writeProject(R"(if engine.isClient then engine.settings.platform = "missing" end)");
    try {
        static_cast<void>(select());
        FAIL() << "an unknown backend was accepted";
    } catch (const RuntimeError& error) {
        const std::string_view message{error.what()};
        EXPECT_NE(message.find("'missing'"), std::string_view::npos) << message;
        EXPECT_NE(message.find("platform"), std::string_view::npos) << message;
    }
}

TEST_F(LibrarySelectionTest, KeepsTheGamePluginsAndWhatTheyLink) {
    writeProject("");
    const std::filesystem::path plugins = _game / Engine::kPluginsDirectory;
    // The fixture under another name: two libraries of the same file name are refused.
    const std::string fixture = libraryFile("rtype-game-fixture");
    std::filesystem::create_directories(plugins);
    std::filesystem::copy_file(
        rtype::core::system::getExecutableDirectory() / "plugins" / libraryFile("rtype-test-plugin"),
        plugins / fixture);
    std::ofstream{plugins / (fixture + ".links")} << libraryFile("rtype-plugin-physics") << '\n';
    const std::vector<SelectedLibrary> selected = select();

    const SelectedLibrary* game = find(selected, "rtype-game-fixture");
    const SelectedLibrary* physics = find(selected, "rtype-plugin-physics");
    ASSERT_NE(game, nullptr);
    ASSERT_NE(physics, nullptr);
    EXPECT_EQ(game->kind, LibraryKind::kGame);
    EXPECT_TRUE(hasReason(*game, "a plugin of the game"));
    EXPECT_TRUE(hasReason(*physics, std::format("linked by {}", fixture)));
}

TEST_F(LibrarySelectionTest, RefusesAGameWithoutAProject) { EXPECT_THROW(static_cast<void>(select()), RuntimeError); }
