/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorCursorTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>

#include "FakeBackend.hpp"
#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/contract/platform/input/Key.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "core/config/Settings.hpp"
#include "editor/EditorPlugin.hpp"
#include "editor/gui/GameViewport.hpp"
#include "editor/gui/GuiContext.hpp"
#include "editor/gui/GuiInput.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/graphics/CursorCapture.hpp"

namespace {
namespace event = aylo::contract::platform::event;
using aylo::contract::platform::Event;
using aylo::contract::platform::input::Key;
using aylo::contract::platform::input::MouseButton;
using aylo::editor::gui::GameViewport;
using aylo::editor::gui::GuiContext;
using aylo::editor::gui::GuiInput;

/// @brief The editor's input bridge on a context of its own, with the game in a panel the mouse is not over.
struct Bridge {
    GuiContext context{std::filesystem::path{}};
    GuiInput input{context.get()};

    Bridge() {
        input.setViewport(
            GameViewport{.position = {100, 100}, .size = {200, 100}, .sceneSize = {200, 100}, .focused = true});
    }

    [[nodiscard]] bool filter(Event event) { return input.filter(event); }
};
}  // namespace

TEST(EditorCursor, LendsTheCursorOnlyToAPlayingAndFocusedGameNotFreedByEscape) {
    EXPECT_TRUE(GuiInput::lendsCursor(true, true, false));
    EXPECT_FALSE(GuiInput::lendsCursor(false, true, false));  // editing, or paused
    EXPECT_FALSE(GuiInput::lendsCursor(true, false, false));  // another panel has the focus
    EXPECT_FALSE(GuiInput::lendsCursor(true, true, true));    // Escape freed it
}

TEST(EditorCursor, GivesTheGameTheWholeMouseWhileItHoldsTheCursor) {
    Bridge bridge;
    // Not captured, and not over the game: the editor's panels have the mouse.
    EXPECT_FALSE(bridge.filter(event::MouseMoved{.position = {10, 10}, .delta = {5, 0}}));
    EXPECT_FALSE(bridge.filter(event::MouseButtonPressed{.button = MouseButton::kLeft}));

    bridge.input.setCursorCaptured(true);
    // Captured: the cursor is hidden and its position means nothing, the moves and buttons are the game's.
    EXPECT_TRUE(bridge.filter(event::MouseMoved{.position = {10, 10}, .delta = {5, 0}}));
    EXPECT_TRUE(bridge.filter(event::MouseButtonPressed{.button = MouseButton::kLeft}));
    EXPECT_TRUE(bridge.filter(event::MouseButtonReleased{.button = MouseButton::kLeft}));
    EXPECT_TRUE(bridge.filter(event::MouseScrolled{.offset = {0, 1}}));
}

TEST(EditorCursor, EscapeFreesTheCaptureAndAClickOnTheGameGivesItBack) {
    Bridge bridge;
    bridge.input.setCursorCaptured(true);
    EXPECT_TRUE(bridge.filter(event::KeyPressed{.key = Key::kEscape}));  // the game still sees its key
    EXPECT_TRUE(bridge.input.isCursorFreed());

    bridge.input.setCursorCaptured(false);  // the editor stopped lending it
    // A click elsewhere keeps it free; a click on the picture of the game gives it back.
    EXPECT_FALSE(bridge.filter(event::MouseButtonPressed{.button = MouseButton::kLeft}));
    EXPECT_TRUE(bridge.input.isCursorFreed());
    GameViewport hovered = bridge.input.getViewport();
    hovered.hovered = true;
    bridge.input.setViewport(hovered);
    EXPECT_TRUE(bridge.filter(event::MouseButtonPressed{.button = MouseButton::kLeft}));
    EXPECT_FALSE(bridge.input.isCursorFreed());
}

TEST(EditorCursor, EscapeFreesNothingWhileTheCursorIsNotCaptured) {
    Bridge bridge;
    static_cast<void>(bridge.filter(event::KeyPressed{.key = Key::kEscape}));
    EXPECT_FALSE(bridge.input.isCursorFreed());
}

TEST(EditorCursor, AGameThatCapturesTheCursorDoesNotWhileTheEditorEdits) {
    auto state = std::make_shared<aylo::test::plugins::FakeBackendState>();
    aylo::core::config::Settings settings = aylo::test::plugins::makeFakeSettings();
    settings.set("window.captureCursor", true);
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    aylo::test::plugins::addDrawingPlugins(app, aylo::test::plugins::makeFakeRegistry(state), settings);
    app.addPlugin(aylo::plugins::capsule::CapsulePlugin{});
    app.addPlugin(aylo::editor::EditorPlugin{
        aylo::editor::EditorOptions{.layoutFile = std::filesystem::path{}, .captureOutput = false}});
    for (int frame = 0; frame < 4; ++frame) {
        app.runFrame();
    }
    EXPECT_TRUE(state->cursorLocks.empty());  // nothing plays yet: the cursor is the editor's
    EXPECT_TRUE(app.getWorld().getResources().get<aylo::plugins::graphics::CursorCapture>()->wanted);
    EXPECT_FALSE(app.getWorld().getResources().get<aylo::plugins::graphics::CursorCapture>()->captured);
}
