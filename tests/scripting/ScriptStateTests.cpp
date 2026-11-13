/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptStateTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <format>
#include <memory>
#include <string>

#include "ScriptProbe.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/state/States.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"

namespace {
/// @brief An App with a `probe` global and a host on it, in which a `game.Phase` machine is declared.
struct Fixture {
    aylo::ecs::App app;
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    Fixture() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::seconds{1}, aylo::ecs::FixedTimestep{1, 5}));
        app.getPluginRegistry().addScriptForms("test.Probe", aylo::tests::kProbeForms);
        auto created = aylo::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
        EXPECT_EQ(run(R"(engine.ecs.state("game.Phase", { "Menu", "Playing", "Over" }))"), "");
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("state-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @return Empty when the systems wrote `expected` in `probe.log`, else the error naming what they wrote.
    [[nodiscard]] std::string expectLog(const std::string& expected) const {
        return run(std::format(R"(assert((probe.log or "") == "{}", probe.log))", expected));
    }

    /// @return The current value of `game.Phase`.
    [[nodiscard]] std::string phase() const {
        const auto value = app.getWorld().getResources().require<aylo::ecs::States>().get("game.Phase");
        return value ? std::string{*value} : std::string{};
    }
};

/// @return Whether `text` contains `part`.
[[nodiscard]] testing::AssertionResult contains(const std::string& text, const std::string& part) {
    if (text.find(part) != std::string::npos) {
        return testing::AssertionSuccess();
    }
    return testing::AssertionFailure() << "'" << text << "' does not contain '" << part << "'";
}

/// @brief Systems that write in `probe.log` when `game.Phase` enters or leaves a value, and on every frame of Playing.
constexpr const char* kLoggers = R"(
    local function write(text) probe.log = (probe.log or "") .. text .. ";" end
    engine.ecs.system("game.EnterMenu", { onEnter = { "game.Phase", "Menu" }, run = function() write("enter Menu") end })
    engine.ecs.system("game.ExitMenu", { onExit = { "game.Phase", "Menu" }, run = function() write("exit Menu") end })
    engine.ecs.system("game.EnterPlaying", {
        onEnter = { "game.Phase", "Playing" },
        run = function() write("enter Playing") end,
    })
    engine.ecs.system("game.Play", {
        phase = "Update",
        inState = { "game.Phase", "Playing" },
        run = function() write("play") end,
    })
)";
}  // namespace

TEST(ScriptState, AMachineStartsInItsFirstValueAndItsSystemsRunOnlyInTheirState) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(kLoggers), "");

    fixture.app.runFrame();
    EXPECT_EQ(fixture.phase(), "Menu");
    EXPECT_EQ(fixture.expectLog("enter Menu;"), "");

    fixture.app.runFrame();
    EXPECT_EQ(fixture.expectLog("enter Menu;"), "");
}

TEST(ScriptState, ASystemChangesTheStateThroughTheEngineCalls) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(kLoggers), "");
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Start", {
            phase = "Update",
            inState = { "game.Phase", "Menu" },
            calls = { "aylo.state.get", "aylo.state.set" },
            run = function(ctx)
                probe.seen = ctx.call("aylo.state.get", { state = "game.Phase" })
                ctx.call("aylo.state.set", { state = "game.Phase", value = "Playing" })
            end,
        })
    )"),
              "");

    fixture.app.runFrame();
    EXPECT_EQ(fixture.phase(), "Menu");
    fixture.app.runFrame();

    EXPECT_EQ(fixture.phase(), "Playing");
    EXPECT_EQ(fixture.expectLog("enter Menu;exit Menu;enter Playing;play;"), "");
    EXPECT_EQ(fixture.run(R"(assert(probe.seen == "Menu", tostring(probe.seen)))"), "");
}

TEST(ScriptState, DeclaringAMachineAgainWithTheSameValuesDoesNothing) {
    const Fixture fixture;
    EXPECT_EQ(fixture.run(R"(engine.ecs.state("game.Phase", { "Menu", "Playing", "Over" }))"), "");
    EXPECT_TRUE(contains(fixture.run(R"(engine.ecs.state("game.Phase", { "Menu" }))"), "game.Phase"));
    EXPECT_TRUE(contains(fixture.run(R"(engine.ecs.state("game.Mode", { "A", 3 }))"), "list of strings"));
}

TEST(ScriptState, AWrongStateOptionIsRefusedWithAMessage) {
    const Fixture fixture;
    EXPECT_TRUE(contains(fixture.run(R"(
        engine.ecs.system("game.A", { phase = "Update", inState = { "game.Nope", "Menu" }, run = function() end })
    )"),
                         "unknown state machine 'game.Nope'"));
    EXPECT_TRUE(contains(fixture.run(R"(
        engine.ecs.system("game.B", { phase = "Update", inState = { "game.Phase", "Paused" }, run = function() end })
    )"),
                         "has no value 'Paused'"));
    EXPECT_TRUE(contains(fixture.run(R"(
        engine.ecs.system("game.C", { phase = "Update", onEnter = { "game.Phase", "Menu" }, run = function() end })
    )"),
                         "it takes no 'phase'"));
    EXPECT_TRUE(contains(fixture.run(R"(
        engine.ecs.system("game.D", { phase = "StateTransition", run = function() end })
    )"),
                         "the StateTransition phase is not written"));
    EXPECT_TRUE(contains(fixture.run(R"(
        engine.ecs.system("game.E", {
            inState = { "game.Phase", "Menu" },
            onExit = { "game.Phase", "Menu" },
            run = function() end,
        })
    )"),
                         "takes one of 'inState', 'onEnter' and 'onExit'"));
    EXPECT_TRUE(contains(fixture.run(R"(
        engine.ecs.system("game.F", { inState = "game.Phase", run = function() end })
    )"),
                         "must be { machine, value }"));
}

TEST(ScriptState, AReplicatedMachineIsTheServers) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(engine.ecs.state("game.Round", { "Warmup", "Live" }, { replicate = true }))"), "");
    auto& states = *fixture.app.getWorld().getResources().getMut<aylo::ecs::States>();
    EXPECT_TRUE(states.isReplicated("game.Round"));
    EXPECT_FALSE(states.isReplicated("game.Phase"));
    // Again as it is: nothing; with another replication: refused.
    EXPECT_EQ(fixture.run(R"(engine.ecs.state("game.Round", { "Warmup", "Live" }, { replicate = true }))"), "");
    EXPECT_TRUE(
        contains(fixture.run(R"(engine.ecs.state("game.Round", { "Warmup", "Live" }))"), "another replication"));
    EXPECT_TRUE(contains(fixture.run(R"(engine.ecs.state("game.Mode", { "A" }, { replicated = true }))"),
                         "unknown option 'replicated'"));
}

TEST(ScriptState, AClientCannotSetAMachineOfTheServer) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(engine.ecs.state("game.Round", { "Warmup", "Live" }, { replicate = true }))"), "");
    fixture.app.getWorld().getResources().getMut<aylo::ecs::States>()->followServer();  // what a client's network does
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Cheat", {
            phase = "Update",
            calls = { "aylo.state.set" },
            run = function(ctx)
                probe.ok, probe.error = pcall(ctx.call, "aylo.state.set", { state = "game.Round", value = "Live" })
            end,
        })
    )"),
              "");
    fixture.app.runFrame();
    EXPECT_EQ(fixture.run(R"(assert(probe.ok == false and string.find(probe.error, "only the server changes it"),
                                    tostring(probe.error)))"),
              "");
}
