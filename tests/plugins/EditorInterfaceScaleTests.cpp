/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorInterfaceScaleTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <memory>
#include <vector>

#include "FakeBackend.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "editor/EditorPlugin.hpp"
#include "editor/gui/GuiContext.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"

namespace {
/// @return The median height, in pixels, of the textured triangles the editor draws in its first frames (its glyphs,
/// mostly; plain shapes sample a single texel), on a window whose desktop asks for @p contentScale.
float medianGlyphHeight(float contentScale) {
    const auto state = std::make_shared<aylo::test::plugins::FakeBackendState>();
    state->contentScale = {contentScale, contentScale};
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    aylo::test::plugins::addDrawingPlugins(app, aylo::test::plugins::makeFakeRegistry(state),
                                           aylo::test::plugins::makeFakeSettings());
    app.addPlugin(aylo::plugins::capsule::CapsulePlugin{});
    app.addPlugin(aylo::editor::EditorPlugin{
        aylo::editor::EditorOptions{.layoutFile = std::filesystem::path{}, .captureOutput = false}});
    for (int frame = 0; frame < 3; ++frame) {
        app.runFrame();
    }
    const aylo::contract::renderer::Mesh& mesh = state->meshes.back();
    std::vector<float> heights;
    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3) {
        const auto& a = mesh.vertices.at(mesh.indices.at(index));
        const auto& b = mesh.vertices.at(mesh.indices.at(index + 1));
        const auto& c = mesh.vertices.at(mesh.indices.at(index + 2));
        if (a.uv == b.uv && b.uv == c.uv) {
            continue;  // a plain shape
        }
        heights.push_back(std::max({a.position.y, b.position.y, c.position.y}) -
                          std::min({a.position.y, b.position.y, c.position.y}));
    }
    if (heights.empty()) {
        return 0.0F;
    }
    std::ranges::nth_element(heights, heights.begin() + static_cast<std::ptrdiff_t>(heights.size() / 2));
    return heights.at(heights.size() / 2);
}
}  // namespace

TEST(EditorInterfaceScale, IsTheContentScaleWhereTheWindowIsInPixels) {
    EXPECT_FLOAT_EQ(aylo::editor::gui::interfaceScale(2.0F, 1.0F), 2.0F);  // X11 or Windows at 200%
    EXPECT_FLOAT_EQ(aylo::editor::gui::interfaceScale(1.5F, 1.0F), 1.5F);
    EXPECT_FLOAT_EQ(aylo::editor::gui::interfaceScale(1.0F, 1.0F), 1.0F);
}

TEST(EditorInterfaceScale, IsOneWhereTheFramebufferAlreadyCoversIt) {
    EXPECT_FLOAT_EQ(aylo::editor::gui::interfaceScale(2.0F, 2.0F), 1.0F);  // macOS or Wayland at 200%
}

TEST(EditorInterfaceScale, ReadsAnUnknownScaleAsOneAndStaysInRange) {
    EXPECT_FLOAT_EQ(aylo::editor::gui::interfaceScale(0.0F, 1.0F), 1.0F);
    EXPECT_FLOAT_EQ(aylo::editor::gui::interfaceScale(-3.0F, 0.0F), 1.0F);
    EXPECT_FLOAT_EQ(aylo::editor::gui::interfaceScale(std::numeric_limits<float>::quiet_NaN(), 1.0F), 1.0F);
    EXPECT_FLOAT_EQ(aylo::editor::gui::interfaceScale(10.0F, 1.0F), 4.0F);
    EXPECT_FLOAT_EQ(aylo::editor::gui::interfaceScale(1.0F, 4.0F), 0.5F);
}

TEST(EditorInterfaceScale, TheContextKeepsTheScaleItApplies) {
    aylo::editor::gui::GuiContext context{std::filesystem::path{}};
    EXPECT_FLOAT_EQ(context.getScale(), 1.0F);
    context.setScale(2.0F);
    EXPECT_FLOAT_EQ(context.getScale(), 2.0F);
    context.setScale(1.0F);
    EXPECT_FLOAT_EQ(context.getScale(), 1.0F);
}

TEST(EditorInterfaceScale, TheEditorDrawsItsTextBiggerOnAScreenAt200Percent) {
    const float normal = medianGlyphHeight(1.0F);
    ASSERT_GT(normal, 0.0F);
    // Not exactly twice: glyphs rendered bigger keep their own padding, and bigger panels show other text.
    EXPECT_GT(medianGlyphHeight(2.0F) / normal, 1.5F);
    EXPECT_FLOAT_EQ(medianGlyphHeight(2.0F), medianGlyphHeight(2.0F));  // the same scale draws the same
}
