/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** NetworkInputTests
*/

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/ecs/type/String.hpp"
#include "aylo/engine/NetworkInput.hpp"
#include "aylo/plugins/GlmReflection.hpp"
#include "aylo/replication/component/Owner.hpp"
#include "aylo/replication/component/PlayerInput.hpp"
#include "aylo/replication/input/InputKind.hpp"
#include "aylo/replication/input/InputLayout.hpp"
#include "aylo/replication/standalone/StandalonePlugin.hpp"

namespace {
using aylo::engine::ActionReading;
using aylo::replication::input::InputKind;
}  // namespace

TEST(NetworkInput, ReadsEachActionOfTheLayoutAsItsKind) {
    aylo::replication::input::InputLayout layout;
    layout.add("shoot", InputKind::kButton);
    layout.add("move", InputKind::kVector2);
    layout.add("throttle", InputKind::kAxis);
    layout.add("dash", InputKind::kButton);  // the reader does not know it: stays neutral

    const aylo::replication::component::PlayerInput read =
        aylo::engine::readNetworkInput(layout, [](const std::string_view name) -> std::optional<ActionReading> {
            if (name == "shoot") {
                return ActionReading{.held = true};
            }
            if (name == "move") {
                return ActionReading{.held = true, .value = glm::vec2{1.0F, 1.0F}};
            }
            if (name == "throttle") {
                return ActionReading{.held = true, .value = glm::vec2{1.0F, 0.0F}};
            }
            return std::nullopt;
        });

    EXPECT_TRUE(layout.isHeld(read, "shoot"));
    EXPECT_FALSE(layout.isHeld(read, "dash"));
    const std::array<float, 2> move = layout.readVector(read, "move");
    EXPECT_GT(move.at(0), 0.0F);
    EXPECT_GT(move.at(1), 0.0F);
    EXPECT_FLOAT_EQ(layout.readAxis(read, "throttle"), 1.0F);
}

TEST(NetworkInput, AddsNoSystemWithoutTheInputCall) {
    aylo::ecs::App app;

    aylo::engine::addNetworkInput(app);  // no input plugin: nothing to read, nothing thrown

    SUCCEED();
}

namespace {
namespace fake_input {
/// @brief The arguments and the result of `aylo.input.read`, named and laid out as the Engine reads them.
struct ReadActionArguments {
    aylo::ecs::String action;
};
AYLO_ECS_CALL_DATA(ReadActionArguments, "aylo.input.ReadActionArguments", action);

struct ActionState {
    glm::vec2 value{0.0F};
    bool held{false};
    bool pressed{false};
    bool released{false};
};
AYLO_ECS_CALL_DATA(ActionState, "aylo.input.ActionState", value, held, pressed, released);

ActionState read(const ReadActionArguments& /*arguments*/) { return ActionState{.value = {1.0F, 0.0F}, .held = true}; }
}  // namespace fake_input
}  // namespace

TEST(NetworkInput, AStandaloneWorldGivesTheInputToTheEntityOfTheLocalClient) {
    aylo::ecs::App app{aylo::ecs::RunMode::kStandalone};
    app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{20}, aylo::ecs::FixedTimestep{50, 5}));
    app.addCall("aylo.input.read", &fake_input::read);
    app.addPlugin(aylo::replication::standalone::StandalonePlugin{});
    app.getWorld().getResources().getMut<aylo::replication::input::InputLayout>()->add("fire", InputKind::kButton);
    aylo::engine::addNetworkInput(app);
    const aylo::ecs::Entity ship = app.getWorld().spawn(aylo::replication::component::Owner{.client = 1},
                                                        aylo::replication::component::PlayerInput{});

    app.runFrame();  // PostUpdate reads the player's input
    app.runFrame();  // the FixedUpdate hands it to the ship

    const auto& layout = *app.getWorld().getResources().get<aylo::replication::input::InputLayout>();
    EXPECT_TRUE(
        layout.isHeld(*app.getWorld().getComponents().get<aylo::replication::component::PlayerInput>(ship), "fire"));
}
