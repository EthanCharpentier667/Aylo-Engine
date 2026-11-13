/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StatesTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/exceptions/StateException.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/state/StateConditions.hpp"
#include "aylo/ecs/state/States.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"

namespace aylo::test::states {
/// @brief What the systems below did, in order.
struct Log {
    std::vector<std::string> entries;  ///< Not reflected: the tests read it from C++.
};
AYLO_ECS_RESOURCE(Log, "test.states.Log");
}  // namespace aylo::test::states

namespace {
using aylo::ecs::App;
using aylo::ecs::Phase;
using aylo::ecs::ResMut;
using aylo::ecs::States;
using aylo::test::states::Log;

void enterMenu(ResMut<Log> log) { log->entries.emplace_back("enter Menu"); }
void exitMenu(ResMut<Log> log) { log->entries.emplace_back("exit Menu"); }
void enterPlaying(ResMut<Log> log) { log->entries.emplace_back("enter Playing"); }
void play(ResMut<Log> log) { log->entries.emplace_back("play"); }

/// @brief An App with a machine "game.Phase" (Menu, then Playing) and systems that log what they see.
struct StatesApp {
    App app;

    StatesApp() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::seconds{1}, aylo::ecs::FixedTimestep{1, 5}));
        app.insertResource(Log{});
        states().add("game.Phase", {"Menu", "Playing"});
        app.addSystem(Phase::kStateTransition, "test.enterMenu", &enterMenu)
            .runIf(aylo::ecs::onEnter("game.Phase", "Menu"))
            .before("test.exitMenu");  // they never run in one pass, but they write the same data: an order is asked
        app.addSystem(Phase::kStateTransition, "test.exitMenu", &exitMenu)
            .runIf(aylo::ecs::onExit("game.Phase", "Menu"));
        app.addSystem(Phase::kStateTransition, "test.enterPlaying", &enterPlaying)
            .runIf(aylo::ecs::onEnter("game.Phase", "Playing"))
            .after("test.exitMenu");  // two systems of one phase writing the log: an order between them
        app.addSystem(Phase::kFixedUpdate, "test.play", &play).runIf(aylo::ecs::inState("game.Phase", "Playing"));
    }

    States& states() { return *app.getWorld().getResources().getMut<States>(); }

    std::vector<std::string> takeLog() {
        return std::exchange(app.getWorld().getResources().getMut<Log>()->entries, {});
    }
};
}  // namespace

TEST(States, EveryAppHasThemEmpty) {
    App app;
    const States* states = app.getWorld().getResources().get<States>();
    ASSERT_NE(states, nullptr);
    EXPECT_FALSE(states->has("game.Phase"));
}

TEST(States, AMachineEntersItsFirstValueOnTheFirstFrame) {
    StatesApp fixture;
    EXPECT_EQ(fixture.states().get("game.Phase"), std::nullopt);  // nothing entered yet

    fixture.app.runFrame();

    EXPECT_EQ(fixture.states().get("game.Phase"), "Menu");
    EXPECT_EQ(fixture.takeLog(), (std::vector<std::string>{"enter Menu"}));
    fixture.app.runFrame();
    EXPECT_TRUE(fixture.takeLog().empty()) << "an onEnter system runs once";
}

TEST(States, ASystemInAStateRunsOnlyInIt) {
    StatesApp fixture;
    fixture.app.runFrame();
    static_cast<void>(fixture.takeLog());
    fixture.app.runFrame();
    EXPECT_TRUE(fixture.takeLog().empty()) << "not Playing yet";

    fixture.states().request("game.Phase", "Playing");
    fixture.app.runFrame();

    EXPECT_EQ(fixture.takeLog(), (std::vector<std::string>{"exit Menu", "enter Playing", "play"}));
    fixture.app.runFrame();
    EXPECT_EQ(fixture.takeLog(), (std::vector<std::string>{"play"}));
}

TEST(States, AChangeIsAppliedAtTheNextFrameAndTheLastRequestWins) {
    StatesApp fixture;
    fixture.app.runFrame();
    fixture.states().request("game.Phase", "Playing");
    fixture.states().request("game.Phase", "Menu");  // back to where it is: nothing to do

    EXPECT_FALSE(fixture.states().hasPending());
    EXPECT_EQ(fixture.states().get("game.Phase"), "Menu");
    fixture.states().request("game.Phase", "Playing");
    EXPECT_EQ(fixture.states().get("game.Phase"), "Menu") << "applied at the next frame, not now";
    fixture.app.runFrame();
    EXPECT_EQ(fixture.states().get("game.Phase"), "Playing");
}

TEST(States, RefusesWhatDoesNotExistAndAnotherDeclaration) {
    States states;
    states.add("game.Phase", {"Menu", "Playing"});
    states.add("game.Phase", {"Menu", "Playing"});  // the same again: a script run again on a hot reload

    EXPECT_THROW(states.add("game.Phase", {"Menu"}), aylo::ecs::exceptions::StateException);
    EXPECT_THROW(states.add("game.Other", {}), aylo::ecs::exceptions::StateException);
    EXPECT_THROW(states.add("game.Other", {"A", "A"}), aylo::ecs::exceptions::StateException);
    EXPECT_THROW(states.request("game.Nope", "Menu"), aylo::ecs::exceptions::StateException);
    EXPECT_THROW(states.request("game.Phase", "Paused"), aylo::ecs::exceptions::StateException);
    EXPECT_FALSE(states.isIn("game.Nope", "Menu"));
}
