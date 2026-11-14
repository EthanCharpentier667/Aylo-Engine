/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptCursorTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "FakeBackend.hpp"
#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/contract/platform/input/Key.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace {
/// @brief A client App with the graphics plugin over the fake backend, and a script host on it.
struct ClientScripts {
    std::shared_ptr<aylo::test::plugins::FakeBackendState> state =
        std::make_shared<aylo::test::plugins::FakeBackendState>();
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    ClientScripts() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{100}));
        app.addPlugin(aylo::plugins::transform::TransformPlugin{});
        app.addPlugin(aylo::plugins::input::InputPlugin{});
        app.addPlugin(aylo::plugins::graphics::GraphicsPlugin{aylo::test::plugins::makeFakeRegistry(state),
                                                              aylo::test::plugins::makeFakeSettings()});
        auto created = aylo::scripting::ScriptHost::create(app, {.isClient = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("cursor-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptCursor, AScriptCapturesTheCursorAndGivesItBack) {
    ClientScripts client;
    ASSERT_EQ(client.run(R"(
        local frames = 0
        engine.ecs.system("game.MouseLook", {
            phase = "Update",
            side = "client",
            calls = { "aylo.graphics.captureCursor" },
            run = function(ctx)
                frames += 1
                if frames == 1 then
                    ctx.call("aylo.graphics.captureCursor", { capture = true })
                elseif frames == 3 then
                    ctx.call("aylo.graphics.captureCursor", { capture = false })
                end
            end,
        })
    )"),
              "");

    for (int frame = 0; frame < 5; ++frame) {
        client.app.runFrame();
    }

    // Captured at the frame after the call, given back at the frame after the second one.
    EXPECT_EQ(client.state->cursorLocks, (std::vector<bool>{true, false}));
}

TEST(ScriptCursor, AScriptReadsWhetherTheCursorIsCaptured) {
    ClientScripts client;
    ASSERT_EQ(client.run(R"(
        engine.ecs.system("game.Look", {
            phase = "Update",
            side = "client",
            resources = { read = { "aylo.CursorCapture" } },
            calls = { "aylo.graphics.captureCursor" },
            run = function(ctx)
                local cursor = ctx.resources["aylo.CursorCapture"]
                if not cursor.wanted then
                    ctx.call("aylo.graphics.captureCursor", { capture = true })
                elseif cursor.captured then
                    error("captured")
                end
            end,
        })
    )"),
              "");

    testing::internal::CaptureStderr();
    for (int frame = 0; frame < 4; ++frame) {
        client.app.runFrame();
    }
    const std::string errors = testing::internal::GetCapturedStderr();
    // The script saw `captured` once the cursor was: its error is the proof, and the game goes on.
    EXPECT_NE(errors.find("captured"), std::string::npos);
}

// The recipe of the manual (engine-calls.md, "Capture the cursor"), as it is written there.
constexpr const char* kManualRecipe = R"(
engine.input.action("look", "vector2", { bind = { { x = "MouseDeltaX", y = "MouseDeltaY" } } })
engine.input.action("menu", "button", { bind = { "Escape" } })

local yaw, pitch = 0, 0
local sensitivity = 0.003 -- radians per pixel

engine.ecs.system("game.MouseLook", {
	phase = "Update",
	side = "client",
	resources = { read = { "aylo.CursorCapture" } },
	calls = { "aylo.input.read", "aylo.graphics.captureCursor" },
	run = function(ctx)
		local cursor = ctx.resources["aylo.CursorCapture"]
		if ctx.call("aylo.input.read", { action = "menu" }).pressed then
			ctx.call("aylo.graphics.captureCursor", { capture = not cursor.wanted })
		end
		if not cursor.captured then
			return -- the menu is open, or the window lost the focus: the mouse is the player's
		end
		local look = ctx.call("aylo.input.read", { action = "look" }).value
		yaw -= look.x * sensitivity
		pitch = math.clamp(pitch + look.y * sensitivity, -1.5, 1.5)
		-- turn your camera's aylo.Transform3D with yaw and pitch here
	end,
})
)";

TEST(ScriptCursor, TheRecipeOfTheManualTogglesTheCaptureWithEscape) {
    ClientScripts client;
    ASSERT_EQ(client.run(kManualRecipe), "");
    using aylo::contract::platform::event::KeyPressed;
    using aylo::contract::platform::event::KeyReleased;
    using aylo::contract::platform::event::MouseMoved;
    using aylo::contract::platform::input::Key;
    client.state->frames = {
        {KeyPressed{.key = Key::kEscape}},                    // captures
        {KeyReleased{.key = Key::kEscape}},                   // applied at the start of this frame
        {MouseMoved{.position = {10, 0}, .delta = {10, 4}}},  // turns the camera, captured
        {KeyPressed{.key = Key::kEscape}},                    // gives it back
        {KeyReleased{.key = Key::kEscape}},
    };

    testing::internal::CaptureStderr();
    for (int frame = 0; frame < 6; ++frame) {
        client.app.runFrame();
    }
    const std::string errors = testing::internal::GetCapturedStderr();

    EXPECT_EQ(errors, "");
    EXPECT_EQ(client.state->cursorLocks, (std::vector<bool>{true, false}));
}
