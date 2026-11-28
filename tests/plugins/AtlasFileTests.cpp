/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AtlasFileTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <string_view>

#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "core/asset/AssetId.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/detail/AtlasFile.hpp"
#include "plugins/sprite/exceptions/SpriteExceptions.hpp"

using aylo::plugins::sprite::AnimationDirection;
using aylo::plugins::sprite::animationId;
using aylo::plugins::sprite::SpriteAtlas;
using aylo::plugins::sprite::detail::parseAtlasFile;

namespace {
/// @brief An Aseprite export of two frames and one tag, as `ships/ship.json` holds it.
constexpr std::string_view kExport = R"({
  "frames": {
    "a": { "frame": { "x": 0, "y": 0, "w": 8, "h": 8 } },
    "b": { "frame": { "x": 8, "y": 0, "w": 8, "h": 8 } }
  },
  "meta": { "image": "ship.png", "frameTags": [ { "name": "blink", "from": 0, "to": 1 } ] }
})";

/// @return The export for `ships/ship.json`; any other path is missing.
std::string readExport(const std::filesystem::path& path) {
    if (path.generic_string() != "ships/ship.json") {
        throw aylo::core::asset::exceptions::AssetLoadException{"file not found: " + path.generic_string()};
    }
    return std::string{kExport};
}

/// @return The atlas the `.atlas` file @p text describes, read as `ships/left.atlas`.
SpriteAtlas parse(std::string_view text) { return parseAtlasFile(text, "ships/left.atlas", &readExport); }

/// @return The message parsing @p text throws, or "" if it does not.
std::string errorOf(std::string_view text) {
    try {
        static_cast<void>(parse(text));
    } catch (const aylo::plugins::sprite::exceptions::AtlasException& error) {
        return error.what();
    }
    return "";
}
}  // namespace

TEST(AtlasFile, CutsAGridOfTheTextureNextToIt) {
    const SpriteAtlas atlas = parse(R"({
      "texture": "sheet.gif",
      "grid": { "frameSize": [48, 48], "count": 5, "spacing": [17, 0], "offset": [9, 9] },
      "animations": { "fly": { "frames": [0, 1, 2, 3, 4], "frameDuration": 0.1 } }
    })");
    EXPECT_EQ(atlas.getTexturePath(), "ships/sheet.gif");
    ASSERT_EQ(atlas.getFrames().size(), 5U);
    EXPECT_FLOAT_EQ(atlas.getFrames()[1].source.getX(), 9.0F + 48.0F + 17.0F);
    EXPECT_FLOAT_EQ(atlas.getFrames()[1].source.getY(), 9.0F);
    EXPECT_EQ(atlas.getTextureDesc().filter, aylo::contract::renderer::TextureFilter::kNearest);
    const auto* fly = atlas.findAnimation(animationId("fly"));
    ASSERT_NE(fly, nullptr);
    EXPECT_EQ(fly->frames.size(), 5U);
    EXPECT_FLOAT_EQ(fly->durations[0], 0.1F);
}

TEST(AtlasFile, AddsListedFramesAfterTheGrid) {
    const SpriteAtlas atlas = parse(R"({
      "texture": "../bullet.gif",
      "linear": true,
      "grid": { "frameSize": [4, 4], "count": 1 },
      "frames": [
        { "position": [108, 151], "size": [80, 16] },
        { "name": "spark", "position": [0, 0], "size": [2, 2], "origin": [0, 1] }
      ]
    })");
    EXPECT_EQ(atlas.getTexturePath(), "bullet.gif");
    EXPECT_EQ(atlas.getTextureDesc().filter, aylo::contract::renderer::TextureFilter::kLinear);
    ASSERT_EQ(atlas.getFrames().size(), 3U);
    EXPECT_EQ(atlas.getFrames()[1].name, "1");
    EXPECT_FLOAT_EQ(atlas.getFrames()[1].origin.x, 40.0F);  // its centre
    EXPECT_EQ(atlas.getFrames()[2].name, "spark");
    EXPECT_FLOAT_EQ(atlas.getFrames()[2].origin.y, 1.0F);
}

TEST(AtlasFile, ReadsEveryFieldOfAnAnimation) {
    const SpriteAtlas atlas = parse(R"({
      "texture": "sheet.png",
      "grid": { "frameSize": [8, 8], "count": 6 },
      "animations": {
        "bounce": { "frames": [4, 2, 0], "direction": "pingPong", "plays": 2 },
        "tail": { "first": 3 },
        "middle": { "first": 1, "count": 2, "direction": "reverse" }
      }
    })");
    const auto* bounce = atlas.findAnimation(animationId("bounce"));
    ASSERT_NE(bounce, nullptr);
    EXPECT_EQ(bounce->frames, (std::vector<std::uint32_t>{4, 2, 0}));
    EXPECT_EQ(bounce->direction, AnimationDirection::kPingPong);
    EXPECT_EQ(bounce->repeat, 2U);
    const auto* tail = atlas.findAnimation(animationId("tail"));
    ASSERT_NE(tail, nullptr);
    EXPECT_EQ(tail->frames, (std::vector<std::uint32_t>{3, 4, 5}));
    const auto* middle = atlas.findAnimation(animationId("middle"));
    ASSERT_NE(middle, nullptr);
    EXPECT_EQ(middle->frames, (std::vector<std::uint32_t>{1, 2}));
    EXPECT_EQ(middle->direction, AnimationDirection::kReverse);
}

TEST(AtlasFile, ReadsAnExportAndAddsItsAnimations) {
    const SpriteAtlas atlas = parse(R"({ "path": "ship.json", "animations": { "still": { "frames": [1] } } })");
    EXPECT_EQ(atlas.getTexturePath(), "ships/ship.png");
    EXPECT_EQ(atlas.getFrames().size(), 2U);
    EXPECT_NE(atlas.findAnimation(animationId("blink")), nullptr);
    EXPECT_NE(atlas.findAnimation(animationId("still")), nullptr);
}

TEST(AtlasFile, NamesTheFileAndTheFieldInItsErrors) {
    EXPECT_EQ(errorOf(R"({ "texture": "a.png", "grdi": {} })"),
              "Atlas 'ships/left.atlas': the atlas file: unknown field 'grdi' (expected 'path', 'texture', 'linear', "
              "'grid', 'frames', 'animations')");
    EXPECT_EQ(errorOf(R"({ "grid": {} })"),
              "Atlas 'ships/left.atlas': expected either a 'path' (an Aseprite or TexturePacker export) or a 'texture' "
              "(an image)");
    EXPECT_EQ(errorOf(R"({ "texture": "a.png", "grid": { "frameSize": [8, 8] } })"),
              "Atlas 'ships/left.atlas': 'grid': expected a 'frameSize' and a 'count' of 1 or more");
    EXPECT_EQ(errorOf(R"({ "texture": "a.png", "frames": [ { "position": [0], "size": [1, 1] } ] })"),
              "Atlas 'ships/left.atlas': frame 1: 'position' must be two numbers, [x, y]");
    EXPECT_EQ(errorOf(R"({ "path": "ship.json", "grid": { "frameSize": [8, 8], "count": 1 } })"),
              "Atlas 'ships/left.atlas': 'grid' and 'frames' cut a 'texture', not an export named by 'path'");
    EXPECT_EQ(
        errorOf(R"({ "texture": "a.png", "grid": { "frameSize": [8, 8], "count": 1 },
                    "animations": { "spin": { "frames": [0], "direction": "around" } } })"),
        "Atlas 'ships/left.atlas': animation 'spin': unknown direction 'around' (expected 'forward', 'reverse' or "
        "'pingPong')");
    EXPECT_NE(errorOf(R"({ "texture": "a.png", "grid": { "frameSize": [8, 8], "count": 1 },
                           "animations": { "far": { "frames": [3] } } })"),
              "");
    EXPECT_NE(errorOf(R"({ "path": "missing.json" })").find("file not found: ships/missing.json"), std::string::npos);
    EXPECT_NE(errorOf("not json").find("Atlas 'ships/left.atlas': "), std::string::npos);
}
