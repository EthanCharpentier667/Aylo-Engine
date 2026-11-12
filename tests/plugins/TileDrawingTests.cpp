/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TileDrawingTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <numbers>
#include <optional>

#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Sprite.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "plugins/graphics/VisibleArea.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "plugins/tilemap/detail/TileDrawing.hpp"

using aylo::contract::renderer::Color;
using aylo::contract::renderer::Sprite;
using aylo::contract::renderer::TextureId;
using aylo::plugins::graphics::VisibleArea;
using aylo::plugins::tilemap::kFlipDiagonal;
using aylo::plugins::tilemap::kFlipHorizontal;
using aylo::plugins::tilemap::kFlipVertical;
using aylo::plugins::tilemap::LayerKind;
using aylo::plugins::tilemap::MapLayer;
using aylo::plugins::tilemap::Tilemap;
using aylo::plugins::tilemap::Tileset;
using aylo::plugins::tilemap::detail::LayerPlacement;

namespace {
constexpr TextureId kTexture{.index = 3, .generation = 1};
constexpr float kQuarterTurn = std::numbers::pi_v<float> / 2.0F;

/// @return A map of 16x16 cells with one tileset of @p tileSize tiles, 4 per row.
Tilemap mapWith(glm::uvec2 tileSize) {
    return Tilemap{{16, 16}, {Tileset{.tileSize = tileSize, .firstGid = 1, .tileCount = 8, .columns = 4}}, {}};
}

/// @return The sprite of the tile id @p gid of @p map in @p cell, placed at the origin with no scale.
Sprite tileOf(const Tilemap& map, std::uint32_t gid, glm::ivec2 cell = {0, 0}) {
    return aylo::plugins::tilemap::detail::tileSprite(map, map.getTilesets().front(), kTexture, gid, cell, {},
                                                      Color{1.0F});
}

/// @return An image layer, repeated along X if @p repeatX.
MapLayer imageLayer(bool repeatX) { return MapLayer{.kind = LayerKind::kImage, .repeatX = repeatX}; }

/// @brief An 800x600 view of the world from its origin.
constexpr VisibleArea kScreen{.min = {0.0F, 0.0F}, .max = {800.0F, 600.0F}};
}  // namespace

TEST(TileDrawing, PlacesATileInItsCellThroughTheLayer) {
    const Tilemap map = mapWith({16, 16});
    const Sprite sprite = aylo::plugins::tilemap::detail::tileSprite(
        map, map.getTilesets().front(), kTexture, 6, {2, 1},
        LayerPlacement{.position = {100.0F, 0.0F}, .scale = {2.0F, 2.0F}}, Color{1.0F});
    EXPECT_EQ(sprite.texture, kTexture);
    EXPECT_EQ(sprite.position, (glm::vec2{180.0F, 48.0F}));  // 100 + (32 + 8) * 2, (16 + 8) * 2.
    EXPECT_EQ(sprite.origin, (glm::vec2{8.0F, 8.0F}));
    EXPECT_EQ(sprite.scale, (glm::vec2{2.0F, 2.0F}));
    EXPECT_FLOAT_EQ(sprite.source.getX(), 16.0F);  // Tile 5: second column, second row.
    EXPECT_FLOAT_EQ(sprite.source.getY(), 16.0F);
}

TEST(TileDrawing, AlignsATallTileOnTheBottomOfItsCell) {
    const Sprite sprite = tileOf(mapWith({16, 32}), 1, {0, 1});
    EXPECT_EQ(sprite.position, (glm::vec2{8.0F, 16.0F}));  // Its top at y 0, a cell above its own.
}

TEST(TileDrawing, FlipsATileHorizontallyAndVertically) {
    const Tilemap map = mapWith({16, 16});
    EXPECT_EQ(tileOf(map, 1U | kFlipHorizontal).scale, (glm::vec2{-1.0F, 1.0F}));
    EXPECT_EQ(tileOf(map, 1U | kFlipVertical).scale, (glm::vec2{1.0F, -1.0F}));
    EXPECT_FLOAT_EQ(tileOf(map, 1U | kFlipVertical).rotation, 0.0F);
}

TEST(TileDrawing, FlipsATileAcrossItsDiagonalWithAQuarterTurn) {
    const Tilemap map = mapWith({16, 16});
    const Sprite diagonal = tileOf(map, 1U | kFlipDiagonal);
    EXPECT_FLOAT_EQ(diagonal.rotation, kQuarterTurn);
    EXPECT_EQ(diagonal.scale, (glm::vec2{1.0F, -1.0F}));
    // Diagonal then horizontal: a quarter turn clockwise, no flip.
    EXPECT_EQ(tileOf(map, 1U | kFlipDiagonal | kFlipHorizontal).scale, (glm::vec2{1.0F, 1.0F}));
}

TEST(TileDrawing, BoundsAChunkInTheWorld) {
    const aylo::plugins::tilemap::detail::Bounds bounds = aylo::plugins::tilemap::detail::chunkBounds(
        mapWith({16, 16}), {16, 0}, LayerPlacement{.position = {10.0F, 0.0F}, .scale = {1.0F, 1.0F}});
    EXPECT_EQ(bounds.min, (glm::vec2{266.0F, 0.0F}));
    EXPECT_EQ(bounds.max, (glm::vec2{522.0F, 256.0F}));
}

TEST(TileDrawing, DrawsAnImageAtItsPlace) {
    const std::optional<Sprite> sprite = aylo::plugins::tilemap::detail::imageSprite(
        imageLayer(false), kTexture, {100, 50}, LayerPlacement{.position = {10.0F, 20.0F}}, kScreen);
    ASSERT_TRUE(sprite);
    EXPECT_EQ(sprite->position, (glm::vec2{10.0F, 20.0F}));
    EXPECT_FLOAT_EQ(sprite->source.getWidth(), 100.0F);
    EXPECT_FLOAT_EQ(sprite->source.getHeight(), 50.0F);
}

TEST(TileDrawing, RepeatsAnImageAcrossTheView) {
    const VisibleArea narrow{.min = {0.0F, 0.0F}, .max = {250.0F, 600.0F}};
    const std::optional<Sprite> sprite = aylo::plugins::tilemap::detail::imageSprite(
        imageLayer(true), kTexture, {100, 50}, LayerPlacement{.position = {30.0F, 0.0F}}, narrow);
    ASSERT_TRUE(sprite);
    EXPECT_FLOAT_EQ(sprite->position.x, -70.0F);         // The copy that covers the left edge.
    EXPECT_FLOAT_EQ(sprite->source.getWidth(), 400.0F);  // Four copies, from -70 to 330.
    EXPECT_FLOAT_EQ(sprite->source.getHeight(), 50.0F);  // Not repeated vertically.
}

TEST(TileDrawing, SkipsAnImageOutOfView) {
    EXPECT_FALSE(aylo::plugins::tilemap::detail::imageSprite(imageLayer(false), kTexture, {100, 50},
                                                             LayerPlacement{.position = {900.0F, 0.0F}}, kScreen));
    EXPECT_FALSE(aylo::plugins::tilemap::detail::imageSprite(imageLayer(true), kTexture, {100, 50}, {}, VisibleArea{}));
}

TEST(TileDrawing, ColoursALayerWithItsTintAndOpacity) {
    const MapLayer layer{.opacity = 0.5F, .tint = Color{1.0F, 0.5F, 1.0F, 1.0F}};
    EXPECT_EQ(aylo::plugins::tilemap::detail::layerColor(layer), (Color{1.0F, 0.5F, 1.0F, 0.5F}));
}
