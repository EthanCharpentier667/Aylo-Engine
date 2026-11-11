/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DirectorySourceTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/vfs/DirectorySource.hpp"
#include "core/vfs/FileSource.hpp"

using aylo::core::asset::exceptions::AssetLoadException;
using aylo::core::vfs::DirectorySource;

namespace {
/// @brief A folder `<root>/assets` with a few files, and a secret file next to it; removed when the test ends.
class DirectorySourceTest : public ::testing::Test {
  protected:
    void SetUp() override {
        write("a.txt", "a");
        write("levels/space.tmj", "{}");
        write("levels/deep/far.tsj", "far");
        std::ofstream{_root / "secret.txt"} << "secret";
    }

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_root, error);
    }

    /// @brief Writes @p text to @p relative, under the assets.
    void write(const std::string& relative, const std::string_view text) const {
        std::filesystem::create_directories((_assets / relative).parent_path());
        std::ofstream{_assets / relative, std::ios::binary} << text;
    }

    std::filesystem::path _root{std::filesystem::temp_directory_path() /
                                std::format("aylo_directory_source_{:08x}", std::random_device{}())};
    std::filesystem::path _assets{_root / "assets"};
};
}  // namespace

TEST_F(DirectorySourceTest, ReadsTheFilesOfItsFolder) {
    const DirectorySource source{_assets};

    EXPECT_EQ(source.read("a.txt"), "a");
    EXPECT_EQ(source.read("levels/../levels/space.tmj"), "{}");
    EXPECT_TRUE(source.exists("levels/deep/far.tsj"));
    EXPECT_FALSE(source.exists("missing.txt"));
    EXPECT_EQ(source.directory(), _assets);
}

TEST_F(DirectorySourceTest, RefusesWhatLeavesItsFolderOrIsMissing) {
    const DirectorySource source{_assets};

    EXPECT_THROW(static_cast<void>(source.read("../secret.txt")), AssetLoadException);
    EXPECT_THROW(static_cast<void>(source.read((_root / "secret.txt").string())), AssetLoadException);
    EXPECT_THROW(static_cast<void>(source.read("missing.txt")), AssetLoadException);
    EXPECT_FALSE(source.exists("../secret.txt"));
}

TEST_F(DirectorySourceTest, ListsItsFilesAtAnyDepthSorted) {
    const DirectorySource source{_assets};

    EXPECT_EQ(source.list(""), (std::vector<std::string>{"a.txt", "levels/deep/far.tsj", "levels/space.tmj"}));
    EXPECT_EQ(source.list("levels/deep"), (std::vector<std::string>{"levels/deep/far.tsj"}));
    EXPECT_TRUE(source.list("missing").empty());
    EXPECT_TRUE(source.list("..").empty());
}

TEST_F(DirectorySourceTest, ReadsBytes) {
    write("bytes.bin", std::string_view{"\x00\xff", 2});
    const std::vector<std::byte> bytes = aylo::core::vfs::readBytes(DirectorySource{_assets}, "bytes.bin");

    ASSERT_EQ(bytes.size(), 2U);
    EXPECT_EQ(bytes[0], std::byte{0x00});
    EXPECT_EQ(bytes[1], std::byte{0xff});
}
