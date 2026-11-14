/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CursorCaptureTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "FakeBackend.hpp"
#include "aylo/contract/platform/IPlatform.hpp"
#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/contract/renderer/IRenderer.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/call/CallInfo.hpp"
#include "aylo/ecs/call/Calls.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "core/config/Settings.hpp"
#include "plugins/graphics/CursorCapture.hpp"
#include "plugins/graphics/ToolFrame.hpp"
#include "plugins/graphics/WindowCalls.hpp"

namespace {
using aylo::contract::platform::event::FocusChanged;
using aylo::plugins::graphics::CaptureCursorArguments;
using aylo::plugins::graphics::CursorCapture;
using aylo::plugins::graphics::kCaptureCursorCall;
using aylo::plugins::graphics::ToolFrame;

/// @brief An App with the GraphicsPlugin over the fake backend, whose `window.captureCursor` is given.
class CursorApp {
  public:
    explicit CursorApp(const bool captureCursor, const aylo::ecs::RunMode mode = aylo::ecs::RunMode::kClient)
        : _state{std::make_shared<aylo::test::plugins::FakeBackendState>()}, _app{mode} {
        aylo::core::config::Settings settings = aylo::test::plugins::makeFakeSettings();
        settings.set("window.captureCursor", captureCursor);
        aylo::test::plugins::addDrawingPlugins(_app, aylo::test::plugins::makeFakeRegistry(_state),
                                               std::move(settings));
    }

    [[nodiscard]] aylo::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] aylo::test::plugins::FakeBackendState& state() noexcept { return *_state; }
    [[nodiscard]] const CursorCapture& cursor() { return *_app.getWorld().getResources().get<CursorCapture>(); }

    /// @brief Makes `aylo.graphics.captureCursor` as a script does.
    void captureCursor(const bool capture) {
        aylo::ecs::World& world = _app.getWorld();
        const aylo::ecs::CallInfo* info = world.getCalls().findByName(kCaptureCursorCall);
        ASSERT_NE(info, nullptr);
        const CaptureCursorArguments arguments{.capture = capture};
        world.getCalls().invoke(world, info->id, std::as_bytes(std::span{&arguments, 1}), std::span<std::byte>{});
    }

    /// @brief Gives the window one frame of focus events, then runs the frame.
    void frameWithFocus(const bool focused) {
        _state->frames.push_back({FocusChanged{.focused = focused}});
        _app.runFrame();
    }

  private:
    std::shared_ptr<aylo::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    aylo::ecs::App _app;                                            ///< The App under test.
};
}  // namespace

TEST(CursorCapture, LeavesTheCursorAloneByDefault) {
    CursorApp app{false};
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_TRUE(app.state().cursorLocks.empty());
    EXPECT_FALSE(app.cursor().captured);
}

TEST(CursorCapture, CapturesItFromTheStartWhenTheSettingAsks) {
    CursorApp app{true};
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_EQ(app.state().cursorLocks, std::vector<bool>{true});
    EXPECT_TRUE(app.cursor().wanted);
    EXPECT_TRUE(app.cursor().captured);
}

TEST(CursorCapture, GivesItBackWhenTheWindowLosesTheFocusAndTakesItAgainWithTheFocus) {
    CursorApp app{true};
    app.app().runFrame();
    app.frameWithFocus(false);
    EXPECT_FALSE(app.cursor().captured);
    EXPECT_TRUE(app.cursor().wanted);  // the game still wants it
    app.frameWithFocus(true);
    EXPECT_EQ(app.state().cursorLocks, (std::vector<bool>{true, false, true}));
    EXPECT_TRUE(app.cursor().captured);
}

TEST(CursorCapture, TheCallCapturesAndReleasesItAtTheNextFrame) {
    CursorApp app{false};
    app.app().runFrame();
    app.captureCursor(true);
    EXPECT_FALSE(app.cursor().captured);
    app.app().runFrame();
    EXPECT_TRUE(app.cursor().captured);
    app.captureCursor(false);
    app.app().runFrame();
    EXPECT_FALSE(app.cursor().captured);
    EXPECT_EQ(app.state().cursorLocks, (std::vector<bool>{true, false}));
}

TEST(CursorCapture, WhileAToolDrawsOverTheGameCapturesItOnlyWhenTheToolLendsIt) {
    CursorApp app{true};
    auto tool = app.app().getWorld().getResources().getMut<ToolFrame>();
    tool->draw = [](aylo::contract::platform::IPlatform&, aylo::contract::renderer::IRenderer&) {};
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_TRUE(app.state().cursorLocks.empty());  // the editor needs its cursor
    EXPECT_FALSE(app.cursor().captured);

    app.app().getWorld().getResources().getMut<ToolFrame>()->lendsCursor = true;  // its Game panel is focused
    app.app().runFrame();
    EXPECT_TRUE(app.cursor().captured);
    app.app().getWorld().getResources().getMut<ToolFrame>()->lendsCursor = false;
    app.app().runFrame();
    EXPECT_FALSE(app.cursor().captured);
    EXPECT_EQ(app.state().cursorLocks, (std::vector<bool>{true, false}));
}

TEST(CursorCapture, TheServerHasTheCallAndNoWindowToCapture) {
    CursorApp app{true, aylo::ecs::RunMode::kServer};
    app.app().runFrame();
    app.captureCursor(true);
    app.app().runFrame();
    EXPECT_TRUE(app.state().cursorLocks.empty());
    EXPECT_FALSE(app.cursor().captured);
}
