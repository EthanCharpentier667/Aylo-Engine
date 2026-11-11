/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PackTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/vfs/DirectorySource.hpp"
#include "core/vfs/Pack.hpp"
#include "core/vfs/exceptions/PackException.hpp"

using aylo::core::asset::exceptions::AssetLoadException;
using aylo::core::vfs::PackEntry;
using aylo::core::vfs::PackSource;
using aylo::core::vfs::SubSource;
using aylo::core::vfs::writePack;
using aylo::core::vfs::exceptions::PackException;

namespace {
/// @brief Appends @p value to @p out, little endian, on @p size bytes: what a test writes a pack by hand with.
void put(std::string& out, const std::uint64_t value, const std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
        out.push_back(static_cast<char>((value >> (8U * i)) & 0xFFU));
    }
}

/// @return The start of a pack written by hand: the magic, @p version and @p count entries.
std::string header(const std::uint32_t version, const std::uint32_t count) {
    std::string bytes{"AYLOPAK", 8};
    put(bytes, version, 4);
    put(bytes, count, 4);
    return bytes;
}

/// @brief Appends to @p bytes an entry of the table: @p path, @p offset and @p size.
void entry(std::string& bytes, const std::string_view path, const std::uint64_t offset, const std::uint64_t size) {
    put(bytes, path.size(), 4);
    bytes += path;
    put(bytes, offset, 8);
    put(bytes, size, 8);
}

/// @brief A folder of its own for the packs of a test, removed when it ends.
class PackTest : public ::testing::Test {
  protected:
    void SetUp() override { std::filesystem::create_directories(_root); }

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_root, error);
    }

    /// @brief Writes @p bytes to the file @p name of the folder.
    [[nodiscard]] std::filesystem::path writeFile(const std::string& name, const std::string_view bytes) const {
        const std::filesystem::path file = _root / name;
        std::ofstream{file, std::ios::binary} << bytes;
        return file;
    }

    /// @return A pack of a small game: its project, a script and two assets, one of them binary.
    [[nodiscard]] std::filesystem::path writeGame() const {
        const std::filesystem::path ship = writeFile("ship.png", std::string_view{"\x89PNG\x00\x01", 6});
        const std::filesystem::path pack = _root / "game.pak";
        writePack(pack, {
                            PackEntry{.path = "project.luau", .data = std::string{"engine.settings.plugins = {}"}},
                            PackEntry{.path = "scripts/main.luau", .data = std::string{"return {}"}},
                            PackEntry{.path = "assets/sprites/ship.png", .data = ship},
                            PackEntry{.path = "assets/levels/../level.tmj", .data = std::string{"{}"}},
                        });
        return pack;
    }

    std::filesystem::path _root{std::filesystem::temp_directory_path() /
                                std::format("aylo_pack_{:08x}", std::random_device{}())};
};
}  // namespace

TEST_F(PackTest, ReadsBackWhatWasWritten) {
    const PackSource pack{writeGame()};

    EXPECT_EQ(pack.read("project.luau"), "engine.settings.plugins = {}");
    EXPECT_EQ(pack.read("scripts/main.luau"), "return {}");
    EXPECT_EQ(pack.read("assets/sprites/ship.png"), (std::string{"\x89PNG\x00\x01", 6}));
    EXPECT_EQ(pack.read("assets/level.tmj"), "{}");  // stored normalized
    EXPECT_EQ(pack.read("assets/sprites/../level.tmj"), "{}");
    EXPECT_FALSE(pack.directory().has_value());
}

TEST_F(PackTest, ListsAndFindsItsFiles) {
    const PackSource pack{writeGame()};

    EXPECT_EQ(pack.list(""), (std::vector<std::string>{"assets/level.tmj", "assets/sprites/ship.png", "project.luau",
                                                       "scripts/main.luau"}));
    EXPECT_EQ(pack.list("assets"), (std::vector<std::string>{"assets/level.tmj", "assets/sprites/ship.png"}));
    EXPECT_TRUE(pack.list("asset").empty());  // a folder is a whole name
    EXPECT_TRUE(pack.exists("scripts/main.luau"));
    EXPECT_FALSE(pack.exists("scripts/other.luau"));
    EXPECT_FALSE(pack.exists("../project.luau"));
}

TEST_F(PackTest, RefusesToReadWhatItDoesNotHold) {
    const PackSource pack{writeGame()};

    EXPECT_THROW(static_cast<void>(pack.read("missing.png")), AssetLoadException);
    EXPECT_THROW(static_cast<void>(pack.read("../project.luau")), AssetLoadException);
    EXPECT_THROW(static_cast<void>(pack.read("/project.luau")), AssetLoadException);
}

TEST_F(PackTest, RefusesToWriteAPathThatLeavesItOrIsGivenTwice) {
    const std::filesystem::path pack = _root / "bad.pak";

    EXPECT_THROW(writePack(pack, {PackEntry{.path = "../out.txt", .data = std::string{}}}), PackException);
    EXPECT_THROW(writePack(pack, {PackEntry{.path = "/abs.txt", .data = std::string{}}}), PackException);
    EXPECT_THROW(writePack(pack, {PackEntry{.path = "", .data = std::string{}}}), PackException);
    EXPECT_THROW(writePack(pack, {PackEntry{.path = "a/b.txt", .data = std::string{}},
                                  PackEntry{.path = "a/./b.txt", .data = std::string{}}}),
                 PackException);
    EXPECT_THROW(writePack(pack, {PackEntry{.path = "a.txt", .data = _root / "missing.txt"}}), PackException);
}

TEST_F(PackTest, OpensAPackAppendedToAnotherFile) {
    const std::filesystem::path pack = writeGame();
    std::ifstream in{pack, std::ios::binary};
    const std::string bytes{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
    const std::string program = "a program comes first";
    const std::filesystem::path joined = writeFile("joined.bin", program + bytes);

    const PackSource appended{joined, program.size()};
    EXPECT_EQ(appended.read("scripts/main.luau"), "return {}");
    EXPECT_THROW(PackSource{joined}, PackException);
}

TEST_F(PackTest, RefusesAFileThatIsNotAPack) {
    EXPECT_THROW(PackSource{_root / "missing.pak"}, PackException);
    EXPECT_THROW(PackSource{writeFile("short.pak", "AYLO")}, PackException);
    EXPECT_THROW(PackSource{writeFile("other.pak", std::string(64, 'x'))}, PackException);
    EXPECT_THROW(PackSource{writeFile("version.pak", header(99, 0))}, PackException);
}

TEST_F(PackTest, RefusesADamagedTable) {
    // More entries than the file could hold: refused before anything is read.
    EXPECT_THROW(PackSource{writeFile("count.pak", header(1, 0xFFFFFFFFU))}, PackException);
    // An entry cut short.
    std::string cut = header(1, 1);
    put(cut, 5, 4);
    cut += "ab";
    EXPECT_THROW(PackSource{writeFile("cut.pak", cut)}, PackException);
    // A path length past what a pack holds.
    std::string longPath = header(1, 1);
    put(longPath, 0xFFFFFFFFU, 4);
    EXPECT_THROW(PackSource{writeFile("length.pak", longPath)}, PackException);
}

TEST_F(PackTest, RefusesAnEntryPastTheEndOrOutsideThePack) {
    std::string past = header(1, 1);
    entry(past, "a.txt", 0, 1'000'000);
    EXPECT_THROW(PackSource{writeFile("past.pak", past)}, PackException);

    std::string overflow = header(1, 1);
    entry(overflow, "a.txt", 0xFFFFFFFFFFFFFFF0ULL, 0x20);
    EXPECT_THROW(PackSource{writeFile("overflow.pak", overflow)}, PackException);

    std::string outside = header(1, 1);
    entry(outside, "../secret.txt", 0, 0);
    EXPECT_THROW(PackSource{writeFile("outside.pak", outside)}, PackException);

    std::string twice = header(1, 2);
    entry(twice, "a.txt", 0, 0);
    entry(twice, "a.txt", 0, 0);
    EXPECT_THROW(PackSource{writeFile("twice.pak", twice)}, PackException);
}

TEST_F(PackTest, ASubSourceIsAFolderOfAnother) {
    const auto pack = std::make_shared<PackSource>(writeGame());
    const SubSource assets{pack, "assets"};

    EXPECT_EQ(assets.read("level.tmj"), "{}");
    EXPECT_EQ(assets.list(""), (std::vector<std::string>{"level.tmj", "sprites/ship.png"}));
    EXPECT_EQ(assets.list("sprites"), (std::vector<std::string>{"sprites/ship.png"}));
    EXPECT_TRUE(assets.exists("sprites/ship.png"));
    EXPECT_FALSE(assets.exists("../project.luau"));
    EXPECT_THROW(static_cast<void>(assets.read("../project.luau")), AssetLoadException);
    EXPECT_FALSE(assets.directory().has_value());
}

TEST_F(PackTest, ASubSourceOfAFolderKeepsItsDirectory) {
    std::filesystem::create_directories(_root / "game" / "assets");
    std::ofstream{_root / "game" / "assets" / "a.txt"} << "a";
    const SubSource assets{std::make_shared<aylo::core::vfs::DirectorySource>(_root / "game"), "assets"};

    EXPECT_EQ(assets.read("a.txt"), "a");
    EXPECT_EQ(assets.list(""), (std::vector<std::string>{"a.txt"}));
    EXPECT_EQ(assets.directory(), _root / "game" / "assets");
}
