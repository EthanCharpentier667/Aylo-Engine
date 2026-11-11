/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** InputPluginTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>

#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/contract/platform/input/Key.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "plugins/input/Control.hpp"
#include "plugins/input/Input.hpp"
#include "plugins/input/InputAction.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/input/PlatformEvent.hpp"

static_assert(aylo::ecs::Resource<aylo::plugins::input::Input>);
static_assert(aylo::ecs::Event<aylo::plugins::input::PlatformEvent>);

namespace {
/// @brief An App with the InputPlugin and a "fire" action bound to Space.
class InputApp {
  public:
    explicit InputApp(aylo::ecs::RunMode mode = aylo::ecs::RunMode::kClient) : _app{mode} {
        _app.addPlugin(aylo::plugins::input::InputPlugin{});
        input()
            .addAction("fire", aylo::plugins::input::ActionType::kButton)
            .bind(aylo::plugins::input::Control::key(aylo::contract::platform::input::Key::kSpace));
    }
    ~InputApp() = default;
    InputApp(const InputApp& other) = delete;
    InputApp& operator=(const InputApp& other) = delete;
    InputApp(InputApp&& other) = delete;
    InputApp& operator=(InputApp&& other) = delete;

    [[nodiscard]] aylo::ecs::App& app() noexcept { return _app; }

    [[nodiscard]] aylo::plugins::input::Input& input() {
        return *_app.getWorld().getResources().requireMut<aylo::plugins::input::Input>();
    }

    /// @brief Sends an event, as the window plugin does after polling the platform.
    template <typename T>
    void send(T event) {
        _app.getWorld().getEvents().send(aylo::plugins::input::PlatformEvent{.event = event});
    }

    [[nodiscard]] const aylo::plugins::input::InputAction& fire() { return input().getAction("fire"); }

  private:
    aylo::ecs::App _app;  ///< The App under test.
};

const aylo::contract::platform::event::KeyPressed kSpacePressed{.key = aylo::contract::platform::input::Key::kSpace};
const aylo::contract::platform::event::KeyReleased kSpaceReleased{.key = aylo::contract::platform::input::Key::kSpace};
}  // namespace

TEST(InputPlugin, InsertsInputAndRegistersTheEvents) {
    aylo::ecs::App app;
    app.addPlugin(aylo::plugins::input::InputPlugin{});
    EXPECT_TRUE(app.hasPlugin("rtype.Input"));
    EXPECT_TRUE(app.getWorld().getResources().has<aylo::plugins::input::Input>());
    EXPECT_TRUE(app.getWorld().getEvents().find("rtype.PlatformEvent"));
}

TEST(InputPlugin, AKeyPressedThisFrameIsPressedThenHeld) {
    InputApp app;
    app.send(kSpacePressed);
    app.app().runFrame();
    EXPECT_TRUE(app.fire().isPressed());
    EXPECT_TRUE(app.fire().isHeld());
    app.app().runFrame();
    EXPECT_FALSE(app.fire().isPressed());
    EXPECT_TRUE(app.fire().isHeld());
}

TEST(InputPlugin, AKeyReleasedIsReleasedTheNextFrame) {
    InputApp app;
    app.send(kSpacePressed);
    app.app().runFrame();
    app.send(kSpaceReleased);
    app.app().runFrame();
    EXPECT_FALSE(app.fire().isHeld());
    EXPECT_TRUE(app.fire().isReleased());
}

TEST(InputPlugin, ATapWithinOneFrameStillCounts) {
    InputApp app;
    app.send(kSpacePressed);
    app.send(kSpaceReleased);
    app.app().runFrame();
    EXPECT_TRUE(app.fire().isPressed());
}

TEST(InputPlugin, AReleaseThenPressWithinOneFrameReadsAsHeld) {
    InputApp app;
    app.send(kSpacePressed);
    app.app().runFrame();
    app.send(kSpaceReleased);
    app.send(kSpacePressed);
    app.app().runFrame();
    EXPECT_TRUE(app.fire().isHeld());
}

TEST(InputPlugin, AnEventIsFedOnce) {
    InputApp app;
    app.send(kSpacePressed);
    app.app().runFrame();
    app.send(kSpaceReleased);
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_FALSE(app.fire().isHeld());
}

TEST(InputPlugin, LosingTheFocusReleasesTheKeys) {
    InputApp app;
    app.send(kSpacePressed);
    app.app().runFrame();
    app.send(aylo::contract::platform::event::FocusChanged{.focused = false});
    app.app().runFrame();
    EXPECT_FALSE(app.fire().isHeld());
}

TEST(InputPlugin, TracksTheMouse) {
    InputApp app;
    app.send(aylo::contract::platform::event::MouseMoved{.position = {12.0F, 34.0F}, .delta = {2.0F, 4.0F}});
    app.app().runFrame();
    EXPECT_EQ(app.input().getMousePosition(), glm::vec2(12.0F, 34.0F));
}

TEST(InputPlugin, CountsEventsWithoutData) {
    InputApp app;
    app.send(aylo::contract::platform::event::GamepadConnected{});
    app.app().runFrame();
    EXPECT_TRUE(app.input().isGamepadConnected());
    app.send(aylo::contract::platform::event::GamepadDisconnected{});
    app.app().runFrame();
    EXPECT_FALSE(app.input().isGamepadConnected());
}

TEST(InputPlugin, DoesNotRunOnADedicatedServer) {
    InputApp app{aylo::ecs::RunMode::kServer};
    app.send(kSpacePressed);
    app.app().runFrame();
    EXPECT_FALSE(app.fire().isHeld());
}

TEST(InputPlugin, IsAddedOnce) {
    aylo::ecs::App app;
    app.addPlugin(aylo::plugins::input::InputPlugin{});
    app.getWorld().getResources().requireMut<aylo::plugins::input::Input>()->addAction(
        "fire", aylo::plugins::input::ActionType::kButton);
    app.addPlugin(aylo::plugins::input::InputPlugin{});
    EXPECT_NO_THROW(
        static_cast<void>(app.getWorld().getResources().require<aylo::plugins::input::Input>().getAction("fire")));
}
