/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StandalonePluginTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/event/EventCursor.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/replication/component/Owner.hpp"
#include "aylo/replication/component/PlayerInput.hpp"
#include "aylo/replication/event/ClientConnected.hpp"
#include "aylo/replication/input/InputKind.hpp"
#include "aylo/replication/input/InputLayout.hpp"
#include "aylo/replication/packet/PacketCalls.hpp"
#include "aylo/replication/packet/PacketSchema.hpp"
#include "aylo/replication/standalone/LocalSession.hpp"
#include "aylo/replication/standalone/StandalonePlugin.hpp"

namespace {
using aylo::ecs::App;
using aylo::ecs::RunMode;
using aylo::replication::component::Owner;
using aylo::replication::component::PlayerInput;
using aylo::replication::event::ClientConnected;
using aylo::replication::input::InputKind;
using aylo::replication::input::InputLayout;
using aylo::replication::standalone::LocalSession;
using aylo::replication::standalone::StandalonePlugin;

constexpr std::chrono::milliseconds kFrame{20};
constexpr std::uint8_t kTickRate = 50;  // one fixed step per frame

/// @return A standalone app with a manual clock, one step per frame, and a "fire" button in the layout.
std::unique_ptr<App> makeApp() {
    auto app = std::make_unique<App>(RunMode::kStandalone);
    app->addPlugin(aylo::ecs::TimePlugin::manual(kFrame, aylo::ecs::FixedTimestep{kTickRate, 5}));
    app->addPlugin(StandalonePlugin{});
    app->getWorld().getResources().getMut<InputLayout>()->add("fire", InputKind::kButton);
    return app;
}

/// @brief Gives the local player's input: "fire" held or not.
void hold(App& app, const bool held) {
    PlayerInput input;
    app.getWorld().getResources().get<InputLayout>()->setButton(input, "fire", held);
    app.getWorld().getResources().getMut<LocalSession>()->setInput(input);
}
}  // namespace

TEST(StandalonePlugin, IsNamedAndRegistersTheVocabulary) {
    const std::unique_ptr<App> holder = makeApp();
    App& app = *holder;

    EXPECT_EQ(StandalonePlugin{}.getName(), "rtype.replication.standalone");
    EXPECT_TRUE(app.hasPlugin("rtype.replication"));
    for (const char* call : {"rtype.net.declarePacket", "rtype.net.sendToServer", "rtype.net.sendToClients",
                             "rtype.replication.localClient", "rtype.replication.readInput"}) {
        EXPECT_NE(app.getWorld().getCalls().findByName(call), nullptr) << call;
    }
    EXPECT_NE(app.getWorld().getEvents().getRegistry().findByName("net.PacketReceived"), nullptr);
    EXPECT_NE(app.getWorld().getComponents().getRegistry().findByName("net.PlayerInput"), nullptr);
}

TEST(StandalonePlugin, RaisesClientConnectedOnceForClientOne) {
    const std::unique_ptr<App> holder = makeApp();
    App& app = *holder;
    aylo::ecs::EventCursor cursor;
    std::vector<std::uint32_t> clients;

    for (int frame = 0; frame < 4; ++frame) {
        app.runFrame();
        for (const ClientConnected& event : app.getWorld().getEvents().read<ClientConnected>(cursor)) {
            clients.push_back(event.client);
        }
    }

    ASSERT_EQ(clients.size(), 1U);
    EXPECT_EQ(clients.front(), 1U);
}

TEST(StandalonePlugin, GivesTheLocalInputToTheEntityOfClientOneWithEdges) {
    const std::unique_ptr<App> holder = makeApp();
    App& app = *holder;
    const aylo::ecs::Entity mine = app.getWorld().spawn(Owner{.client = 1}, PlayerInput{});
    const aylo::ecs::Entity other = app.getWorld().spawn(Owner{.client = 2}, PlayerInput{});
    const InputLayout& layout = *app.getWorld().getResources().get<InputLayout>();
    const auto fire = [&](const aylo::ecs::Entity entity) {
        return layout.isHeld(*app.getWorld().getComponents().get<PlayerInput>(entity), "fire");
    };
    const auto pressed = [&] {
        return layout.isPressed(*app.getWorld().getComponents().get<PlayerInput>(mine), "fire");
    };
    const auto released = [&] {
        return layout.isReleased(*app.getWorld().getComponents().get<PlayerInput>(mine), "fire");
    };

    hold(app, true);
    app.runFrame();
    EXPECT_TRUE(fire(mine));
    EXPECT_TRUE(pressed());
    EXPECT_FALSE(fire(other));

    app.runFrame();  // still held: no longer a press
    EXPECT_TRUE(fire(mine));
    EXPECT_FALSE(pressed());

    hold(app, false);
    app.runFrame();
    EXPECT_FALSE(fire(mine));
    EXPECT_TRUE(released());
}
