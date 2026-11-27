/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptSystemFormsTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/contract/platform/input/Key.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/replication/standalone/StandalonePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/input/PlatformEvent.hpp"

namespace {
/// @brief A single-player App with the input and standalone plugins, and a script host on it.
struct Scripts {
    aylo::ecs::App app{aylo::ecs::RunMode::kStandalone};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    Scripts() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{20}, aylo::ecs::FixedTimestep{50, 5}));
        app.addPlugin(aylo::plugins::input::InputPlugin{});
        app.addPlugin(aylo::replication::standalone::StandalonePlugin{});
        auto created = aylo::scripting::ScriptHost::create(app, {.isClient = true, .isServer = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("system-forms-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @brief Sends a key press, as the backend plugin does after polling the window.
    void press(const aylo::contract::platform::input::Key key) {
        app.getWorld().getEvents().send(
            aylo::plugins::input::PlatformEvent{.event = aylo::contract::platform::event::KeyPressed{.key = key}});
    }
};
}  // namespace

TEST(ScriptSystemForms, InputReadGivesThisMachinesActionOrAPlayersOne) {
    Scripts scripts;
    ASSERT_EQ(scripts.run(R"lua(
        engine.input.action("shoot", nil, { bind = { "Space" } })
        engine.input.action("move", "vector2", { bind = { { up = "W", down = "S", left = "A", right = "D" } } })
        engine.ecs.component("game.Seen", { fields = { local_ = "bool", player = "bool" } })

        local Gun = engine.behaviour("game.Gun", { fields = { shots = "f32" }, side = "both" })
        function Gun:onUpdate()
            if engine.input.read(self, "shoot").pressed then
                self.shots += 1
            end
        end

        engine.ecs.system("game.Read", {
            phase = "FixedUpdate",
            query = { with = { "net.PlayerInput" } },
            after = { "net.applyInputs" },
            run = function(ctx)
                for entity in ctx.query do
                    local move = engine.input.read(ctx, entity, "move")
                    assert(move.vector ~= nil and move.held == false, "the player's input")
                    local ok, message = pcall(engine.input.read, ctx, entity)
                    assert(not ok and string.find(message, "engine.input.read: expected the action's name after the entity", 1, true), message)
                    ok, message = pcall(engine.input.read, ctx, 3)
                    assert(not ok and string.find(message, "expected the action's name, or an entity and the action's name, after ctx", 1, true), message)
                    ok, message = pcall(engine.input.read, ctx, "nope")
                    assert(not ok and string.find(message, "engine.input.read: ", 1, true), message)
                    ok, message = pcall(engine.input.read, nil, "shoot")
                    assert(not ok and string.find(message, "expected the system's ctx (or a Behaviour's self) first", 1, true), message)
                end
            end,
        })
        engine.ecs.system("game.Spawn", {
            phase = "Startup",
            run = function(ctx)
                ctx.commands:spawn({ ["game.Gun"] = {}, ["net.Owner"] = { client = 1 }, ["net.PlayerInput"] = {} })
            end,
        })
    )lua"),
              "");
    testing::internal::CaptureStderr();
    scripts.app.runFrame();
    scripts.press(aylo::contract::platform::input::Key::kSpace);
    scripts.app.runFrame();
    scripts.app.runFrame();
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
    ASSERT_EQ(scripts.run(R"(
        engine.ecs.system("game.Check", {
            phase = "Update",
            query = { read = { "game.Gun" } },
            after = { "game.Gun.onUpdate" },
            run = function(ctx)
                for _, gun in ctx.query do assert(gun.shots == 1, gun.shots) end
            end,
        })
    )"),
              "");
    testing::internal::CaptureStderr();
    scripts.app.runFrame();
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
}

TEST(ScriptSystemForms, LocalClientSaysWhoThisClientIs) {
    Scripts scripts;
    ASSERT_EQ(scripts.run(R"lua(
        engine.ecs.system("game.Who", {
            side = "client",
            phase = "Update",
            run = function(ctx)
                local me = engine.net.localClient(ctx)
                assert(me.connected and me.client == 1, "a standalone world is client 1 at once")
                local ok, message = pcall(engine.net.localClient, ctx, 2)
                assert(not ok and string.find(message, "engine.net.localClient: expected at most 1 argument(s)", 1, true), message)
            end,
        })
    )lua"),
              "");
    testing::internal::CaptureStderr();
    scripts.app.runFrame();
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
}
