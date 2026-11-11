/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GraphicsToolsTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <variant>

#include "FakeBackend.hpp"
#include "aylo/contract/platform/IPlatform.hpp"
#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/contract/platform/input/Key.hpp"
#include "aylo/contract/renderer/IRenderer.hpp"
#include "aylo/contract/renderer/drawing/Mesh.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/event/EventCursor.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsResources.hpp"
#include "plugins/graphics/OverlayQueue.hpp"
#include "plugins/graphics/PlatformEventFilter.hpp"
#include "plugins/graphics/ToolFrame.hpp"
#include "plugins/graphics/VisibleArea.hpp"
#include "plugins/graphics/WindowEvents.hpp"  // IWYU pragma: keep
#include "plugins/input/InputEvents.hpp"      // IWYU pragma: keep
#include "plugins/input/PlatformEvent.hpp"

namespace {
namespace event = aylo::contract::platform::event;
using aylo::plugins::graphics::PlatformEventFilter;
using aylo::plugins::graphics::ToolFrame;

/// @brief An App with the GraphicsPlugin over the fake platform and renderer, as a client has it.
class ToolApp {
  public:
    ToolApp() : _state{std::make_shared<aylo::test::plugins::FakeBackendState>()}, _app{aylo::ecs::RunMode::kClient} {
        aylo::test::plugins::addDrawingPlugins(_app, aylo::test::plugins::makeFakeRegistry(_state),
                                               aylo::test::plugins::makeFakeSettings());
    }
    ~ToolApp() = default;
    ToolApp(const ToolApp& other) = delete;
    ToolApp& operator=(const ToolApp& other) = delete;
    ToolApp(ToolApp&& other) = delete;
    ToolApp& operator=(ToolApp&& other) = delete;

    [[nodiscard]] aylo::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] aylo::test::plugins::FakeBackendState& state() noexcept { return *_state; }

  private:
    std::shared_ptr<aylo::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    aylo::ecs::App _app;                                            ///< The App under test.
};
}  // namespace

TEST(GraphicsTools, LetsEveryEventThroughWithoutAFilter) {
    ToolApp app;
    app.state().frames.push_back({event::KeyPressed{.key = aylo::contract::platform::input::Key::kA},
                                  event::MouseMoved{.position = {10.0F, 20.0F}}});
    app.app().runFrame();
    EXPECT_EQ(app.app().getWorld().getEvents().getCount<event::KeyPressed>(), 1U);
    EXPECT_EQ(app.app().getWorld().getEvents().getCount<event::MouseMoved>(), 1U);
}

TEST(GraphicsTools, TheEventFilterCanHideAnEvent) {
    ToolApp app;
    app.app().getWorld().getResources().getMut<PlatformEventFilter>()->filter =
        [](aylo::contract::platform::Event& received) { return !std::holds_alternative<event::KeyPressed>(received); };
    app.state().frames.push_back(
        {event::KeyPressed{.key = aylo::contract::platform::input::Key::kA}, event::TextEntered{.codepoint = U'a'}});
    app.app().runFrame();
    EXPECT_EQ(app.app().getWorld().getEvents().getCount<event::KeyPressed>(), 0U);
    EXPECT_EQ(app.app().getWorld().getEvents().getCount<event::TextEntered>(), 1U);
    aylo::ecs::EventCursor cursor;
    EXPECT_EQ(app.app().getWorld().getEvents().read<aylo::plugins::input::PlatformEvent>(cursor).size(), 1U);
}

TEST(GraphicsTools, TheEventFilterCanChangeAnEvent) {
    ToolApp app;
    app.app().getWorld().getResources().getMut<PlatformEventFilter>()->filter =
        [](aylo::contract::platform::Event& received) {
            if (auto* moved = std::get_if<event::MouseMoved>(&received)) {
                moved->position -= glm::vec2{100.0F, 50.0F};
            }
            return true;
        };
    app.state().frames.push_back({event::MouseMoved{.position = {110.0F, 70.0F}}});
    app.app().runFrame();
    aylo::ecs::EventCursor cursor;
    const auto moves = app.app().getWorld().getEvents().read<event::MouseMoved>(cursor);
    ASSERT_EQ(moves.size(), 1U);
    EXPECT_EQ(moves.begin()->position, glm::vec2(10.0F, 20.0F));
}

TEST(GraphicsTools, TheRendererFollowsTheWindowWhateverTheFilterDecides) {
    ToolApp app;
    app.app().getWorld().getResources().getMut<PlatformEventFilter>()->filter =
        [](aylo::contract::platform::Event& /*received*/) { return false; };
    app.state().frames.push_back({event::Resized{.width = 1024, .height = 768}});
    app.app().runFrame();
    ASSERT_EQ(app.state().resizes.size(), 1U);
    EXPECT_EQ(app.state().resizes[0], glm::uvec2(1024, 768));
    EXPECT_EQ(app.app().getWorld().getEvents().getCount<event::Resized>(), 0U);
}

TEST(GraphicsTools, CallsTheToolFrameOncePerFrameBeforeTheFrameEnds) {
    ToolApp app;
    int calls = 0;
    int endedWhenCalled = -1;
    aylo::test::plugins::FakeBackendState& state = app.state();
    app.app().getWorld().getResources().getMut<ToolFrame>()->draw =
        [&](aylo::contract::platform::IPlatform& /*platform*/, aylo::contract::renderer::IRenderer& renderer) {
            ++calls;
            endedWhenCalled = state.endFrames;
            renderer.drawMesh(aylo::contract::renderer::Mesh{});
        };
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_EQ(calls, 2);
    EXPECT_EQ(endedWhenCalled, 1);  // The second call saw the first frame ended, not its own.
    EXPECT_EQ(state.meshes.size(), 2U);
    EXPECT_EQ(state.endFrames, 2);
}

TEST(GraphicsTools, DoesNotCallTheToolFrameWhileMinimized) {
    ToolApp app;
    int calls = 0;
    app.state().framebufferSize = {0, 0};
    app.app().getWorld().getResources().getMut<ToolFrame>()->draw =
        [&calls](aylo::contract::platform::IPlatform& /*platform*/, aylo::contract::renderer::IRenderer& /*renderer*/) {
            ++calls;
        };
    app.app().runFrame();
    EXPECT_EQ(calls, 0);
}

TEST(GraphicsTools, TheSceneTextureSizeIsTheFramebufferTheGameSees) {
    ToolApp app;
    app.state().sceneSize = glm::uvec2{640, 360};
    app.app().runFrame();
    const auto& info = *app.app().getWorld().getResources().get<aylo::plugins::graphics::ViewInfo>();
    EXPECT_EQ(info.framebuffer, glm::uvec2(640, 360));
    EXPECT_EQ(info.window, glm::uvec2(640, 360));
    EXPECT_EQ(info.viewSize, glm::vec2(640.0F, 360.0F));
}

TEST(GraphicsTools, TheVisibleAreaFollowsTheSceneTexture) {
    ToolApp app;
    app.state().sceneSize = glm::uvec2{640, 360};
    app.app().runFrame();
    const auto& visible = *app.app().getWorld().getResources().get<aylo::plugins::graphics::VisibleArea>();
    EXPECT_EQ(visible.max - visible.min, glm::vec2(640.0F, 360.0F));
}

TEST(GraphicsTools, TheGameSeesTheWindowWhenNothingAsksForASceneTexture) {
    ToolApp app;
    app.app().runFrame();
    const auto& info = *app.app().getWorld().getResources().get<aylo::plugins::graphics::ViewInfo>();
    EXPECT_EQ(info.framebuffer, glm::uvec2(800, 600));
}
