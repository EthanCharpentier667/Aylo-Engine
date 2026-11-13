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
#include <string_view>
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

TEST(States, AWorldThatFollowsTheServerWaitsForItsValueAndRunsTheSameTransitions) {
    StatesApp follower;
    follower.states().followServer();
    // Declared before following (by a script) or after (below): both wait for the server.
    follower.states().add("game.Round", {"Warmup", "Live"}, States::Replication::kFromServer);
    follower.app.runFrame();
    EXPECT_FALSE(follower.states().get("game.Round").has_value());

    EXPECT_TRUE(follower.states().follow("game.Round", "Live"));
    follower.app.runFrame();
    EXPECT_EQ(follower.states().get("game.Round"), "Live");
}

TEST(States, AMachineDeclaredBeforeFollowingWaitsToo) {
    StatesApp follower;
    follower.states().add("game.Round", {"Warmup", "Live"}, States::Replication::kFromServer);
    follower.states().followServer();
    follower.app.runFrame();
    EXPECT_FALSE(follower.states().get("game.Round").has_value());
    EXPECT_EQ(follower.states().get("game.Phase"), "Menu");  // a local machine starts as always
}

TEST(States, OnlyTheServerChangesAReplicatedMachine) {
    StatesApp server;
    server.states().add("game.Round", {"Warmup", "Live"}, States::Replication::kFromServer);
    server.states().request("game.Round", "Live");               // the server owns it
    EXPECT_FALSE(server.states().follow("game.Round", "Live"));  // and follows no one

    StatesApp client;
    client.states().add("game.Round", {"Warmup", "Live"}, States::Replication::kFromServer);
    client.states().followServer();
    EXPECT_THROW(client.states().request("game.Round", "Live"), aylo::ecs::exceptions::StateException);
    client.states().request("game.Phase", "Playing");               // its own machines still change
    EXPECT_FALSE(client.states().follow("game.Phase", "Playing"));  // the server does not own it
    EXPECT_FALSE(client.states().follow("game.Nope", "Live"));
    EXPECT_FALSE(client.states().follow("game.Round", "Paused"));
}

TEST(States, ListsTheReplicatedMachinesAndRefusesAnotherReplicationOrALongValue) {
    States states;
    states.add("game.Phase", {"Menu"});
    states.add("game.Round", {"Warmup", "Live"}, States::Replication::kFromServer);
    EXPECT_EQ(states.getReplicated(), std::vector<std::string_view>{"game.Round"});
    EXPECT_TRUE(states.isReplicated("game.Round"));
    EXPECT_FALSE(states.isReplicated("game.Phase"));
    EXPECT_NO_THROW(states.add("game.Round", {"Warmup", "Live"}, States::Replication::kFromServer));
    EXPECT_THROW(states.add("game.Round", {"Warmup", "Live"}), aylo::ecs::exceptions::StateException);
    EXPECT_THROW(states.add("game.Long", {std::string(States::kMaxLength + 1, 'a')}),
                 aylo::ecs::exceptions::StateException);
    EXPECT_THROW(states.add(std::string(States::kMaxLength + 1, 'a'), {"A"}), aylo::ecs::exceptions::StateException);
}
