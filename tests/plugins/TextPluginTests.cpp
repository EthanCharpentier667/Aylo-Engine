/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TextPluginTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <utility>
#include <vector>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "aylo/contract/renderer/drawing/Text.hpp"
#include "aylo/contract/renderer/math/Rect.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/ecs/world/World.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/sprite/SpriteRenderer.hpp"
#include "plugins/text/Font.hpp"
#include "plugins/text/FontAssets.hpp"
#include "plugins/text/TextLayout.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/text/TextRenderer.hpp"
#include "plugins/transform/Transform.hpp"

using aylo::contract::renderer::math::Rect;
using aylo::core::asset::AssetId;
using aylo::plugins::text::Font;
using aylo::plugins::text::FontAssets;
using aylo::plugins::text::Glyph;
using aylo::plugins::text::TextRenderer;
using aylo::plugins::transform::Transform;

namespace {
/// @brief An App with the GraphicsPlugin over the fake backend, and the font "ui": an 'A' one em high and half an em
/// wide, the baseline one em down, lines 1.5 ems apart, its atlas font.png (2x1 pixels) loaded.
class TextApp {
  public:
    TextApp() : _state{std::make_shared<aylo::test::plugins::FakeBackendState>()}, _app{aylo::ecs::RunMode::kClient} {
        aylo::test::plugins::addDrawingPlugins(_app, aylo::test::plugins::makeFakeRegistry(_state),
                                               aylo::test::plugins::makeFakeSettings());
        _app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::seconds{1}));
        static_cast<void>(world().getResources().getMut<aylo::plugins::graphics::TextureAssets>()->load(
            "font.png", aylo::test::plugins::twoPixelPng()));
        Font font{"font.png", {}, 3.0F, Font::Metrics{.lineHeight = 1.5F, .ascender = -1.0F, .descender = 0.5F}};
        font.addGlyph(U'A', Glyph{.advance = 0.5F,
                                  .plane = Rect{{0.0F, -1.0F}, {0.5F, 1.0F}},
                                  .atlas = Rect{{0.0F, 0.0F}, {1.0F, 1.0F}}});
        _font = world().getResources().getMut<FontAssets>()->add("ui", font);
    }
    ~TextApp() = default;
    TextApp(const TextApp& other) = delete;
    TextApp& operator=(const TextApp& other) = delete;
    TextApp(TextApp&& other) = delete;
    TextApp& operator=(TextApp&& other) = delete;

    [[nodiscard]] aylo::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] aylo::ecs::World& world() noexcept { return _app.getWorld(); }
    [[nodiscard]] aylo::test::plugins::FakeBackendState& state() noexcept { return *_state; }
    [[nodiscard]] AssetId font() const noexcept { return _font; }

    /// @brief Spawns an entity at @p position drawing @p text.
    aylo::ecs::Entity spawnText(glm::vec2 position, TextRenderer text) {
        const aylo::ecs::Entity entity = world().spawn();
        world().getComponents().add(entity, Transform{.position = position});
        world().getComponents().add(entity, std::move(text));
        return entity;
    }

    /// @return The texts drawn by one more frame.
    std::vector<aylo::contract::renderer::Text> drawFrame() {
        _state->texts.clear();
        _state->drawOrder.clear();
        _app.runFrame();
        return _state->texts;
    }

  private:
    std::shared_ptr<aylo::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    aylo::ecs::App _app;                                            ///< The App under test.
    AssetId _font;                                                  ///< The font "ui".
};
}  // namespace

TEST(TextPlugin, ComesWithTheGraphicsPlugin) {
    TextApp app;
    EXPECT_TRUE(app.app().hasPlugin(aylo::plugins::text::TextPlugin::kName));
    EXPECT_TRUE(app.world().getResources().has<FontAssets>());
    EXPECT_TRUE(app.world().getComponents().find("aylo.TextRenderer"));
}

TEST(TextPlugin, DrawsATextAtItsEntity) {
    TextApp app;
    static_cast<void>(app.spawnText(
        {100.0F, 50.0F},
        TextRenderer{.text = "AA", .font = app.font(), .size = 10.0F, .color = {1.0F, 0.0F, 0.0F, 1.0F}}));
    const auto texts = app.drawFrame();
    ASSERT_EQ(texts.size(), 1U);
    const aylo::contract::renderer::Text& text = texts.at(0);
    EXPECT_EQ(text.position, (glm::vec2{100.0F, 50.0F}));
    EXPECT_EQ(text.glyphs.size(), 2U);
    EXPECT_FLOAT_EQ(text.glyphs.at(1).bounds.getX(), 5.0F);
    EXPECT_FLOAT_EQ(text.distanceRange, 3.0F);
    EXPECT_EQ(text.color, (glm::vec4{1.0F, 0.0F, 0.0F, 1.0F}));
    EXPECT_EQ(text.origin, (glm::vec2{0.0F}));
}

TEST(TextPlugin, DrawsATextWithItsMaterialAndInstanceValues) {
    TextApp app;
    app.state().shaders = true;
    aylo::ecs::World& world = app.app().getWorld();
    auto& shaders = *world.getResources().getMut<aylo::plugins::shader::ShaderAssets>();
    const aylo::core::asset::AssetId shader =
        shaders.add("flash", "instance uniform float flash = 0.25;\nvoid fragment() { COLOR.rgb += flash; }\n");
    const aylo::core::asset::AssetId material =
        world.getResources().getMut<aylo::plugins::shader::MaterialAssets>()->add("hit", {.shader = shader});
    const aylo::ecs::Entity entity =
        app.spawnText({0.0F, 0.0F}, TextRenderer{.text = "A", .font = app.font(), .material = material});
    const aylo::contract::renderer::CompiledShader& compiled = *shaders.get(shader);
    world.getComponents().add(entity,
                              aylo::plugins::shader::MaterialInstance::of(compiled).set(compiled, "flash", 0.75F));
    const auto texts = app.drawFrame();
    ASSERT_EQ(texts.size(), 1U);
    EXPECT_NE(texts.at(0).material, aylo::contract::renderer::MaterialId{});
    EXPECT_FLOAT_EQ(texts.at(0).instanceValues[0].x, 0.75F);
}

TEST(TextPlugin, DrawsATextWithTheDefaultMaterialUntilItsIsCreated) {
    TextApp app;
    static_cast<void>(app.spawnText(
        {0.0F, 0.0F},
        TextRenderer{.text = "A", .font = app.font(), .material = aylo::core::asset::AssetId::fromPath("hit")}));
    const auto texts = app.drawFrame();
    ASSERT_EQ(texts.size(), 1U);
    EXPECT_EQ(texts.at(0).material, aylo::contract::renderer::MaterialId{});
}

TEST(TextPlugin, PivotsATextOnItsAnchor) {
    TextApp app;
    static_cast<void>(app.spawnText(
        {0.0F, 0.0F}, TextRenderer{.text = "AA", .font = app.font(), .size = 10.0F, .anchor = {0.5F, 1.0F}}));
    const auto texts = app.drawFrame();
    ASSERT_EQ(texts.size(), 1U);
    EXPECT_EQ(texts.at(0).origin, (glm::vec2{5.0F, 15.0F}));  // Half of 10 wide, all of one 15-high line.
}

TEST(TextPlugin, SkipsATextWithoutItsFont) {
    TextApp app;
    static_cast<void>(app.spawnText({0.0F, 0.0F}, TextRenderer{.text = "AA", .font = AssetId::fromPath("missing")}));
    static_cast<void>(app.spawnText({0.0F, 0.0F}, TextRenderer{.font = app.font()}));  // And an empty one.
    EXPECT_TRUE(app.drawFrame().empty());
}

TEST(TextPlugin, SortsTheTextsWithTheSpritesByLayer) {
    TextApp app;
    static_cast<void>(app.spawnText({0.0F, 0.0F}, TextRenderer{.text = "A", .font = app.font(), .layer = 10}));
    const aylo::ecs::Entity ship = app.world().spawn();
    app.world().getComponents().add(ship, Transform{});
    app.world().getComponents().add(
        ship, aylo::plugins::sprite::SpriteRenderer{.texture = AssetId::fromPath("font.png"), .layer = 5});
    static_cast<void>(app.drawFrame());
    EXPECT_EQ(app.state().drawOrder, (std::vector<char>{'s', 't'}));  // The text, layer 10, on top.
}
