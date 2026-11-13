/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ClientPluginTests
*/

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <thread>

#include "ServerFixtures.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/call/CallInfo.hpp"
#include "aylo/ecs/call/Calls.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/exceptions/StateException.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/state/States.hpp"
#include "aylo/ecs/system/EventReader.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/network/NetworkSettings.hpp"
#include "aylo/network/client/NetClient.hpp"
#include "aylo/network/io/Address.hpp"
#include "aylo/replication/ReplicationPlugin.hpp"
#include "aylo/replication/ReplicationSettings.hpp"
#include "aylo/replication/client/ClientCalls.hpp"
#include "aylo/replication/client/ClientPlugin.hpp"
#include "aylo/replication/client/ClientState.hpp"
#include "aylo/replication/component/NetworkId.hpp"
#include "aylo/replication/component/Owner.hpp"
#include "aylo/replication/component/PlayerInput.hpp"
#include "aylo/replication/event/ClientConnected.hpp"
#include "aylo/replication/input/InputKind.hpp"
#include "aylo/replication/input/InputLayout.hpp"
#include "aylo/replication/server/ServerPlugin.hpp"
#include "aylo/replication/server/ServerState.hpp"

namespace {
namespace network = aylo::network;
using namespace std::chrono_literals;  // NOLINT(google-build-using-namespace): only the literals
using aylo::ecs::App;
using aylo::ecs::Commands;
using aylo::ecs::EventReader;
using aylo::ecs::FixedTimestep;
using aylo::ecs::Phase;
using aylo::ecs::Presence;
using aylo::ecs::Query;
using aylo::ecs::ReplicationMode;
using aylo::ecs::ResMut;
using aylo::ecs::RunMode;
using aylo::ecs::TimePlugin;
using aylo::replication::ReplicationPlugin;
using aylo::replication::ReplicationSettings;
using aylo::replication::client::ClientPlugin;
using aylo::replication::client::ClientState;
using aylo::replication::client::ConnectionStatus;
using aylo::replication::client::ManifestStatus;
using aylo::replication::component::NetworkId;
using aylo::replication::component::Owner;
using aylo::replication::component::PlayerInput;
using aylo::replication::event::ClientConnected;
using aylo::replication::input::InputKind;
using aylo::replication::input::InputLayout;
using aylo::replication::server::ServerPlugin;
using aylo::replication::server::ServerState;
using aylo::test::server::Position;
using aylo::test::server::Secret;

constexpr std::uint8_t kTickRate = 50;
constexpr std::chrono::milliseconds kFrame{20};

network::NetworkSettings makeNetworkSettings() {
    network::NetworkSettings settings;
    settings.port = 0;
    return settings;
}

/// @brief Game (server): spawns a ship for each client that joins.
void spawnShips(EventReader<ClientConnected> joined, Commands& commands) {
    for (const ClientConnected& event : joined) {
        commands.spawn().add(Owner{.client = event.client}).add(PlayerInput{}).add(Position{.x = 1.F, .y = 2.F});
    }
}

/// @brief Game (client): holds the fire button.
void pressFire(ResMut<ClientState> state) { state->setInput(PlayerInput{.buttons = 1}); }

/// @brief Game (server): moves the ships.
void moveShips(Query<Position> positions) {
    for (auto [entity, position] : positions) {
        position->x += 1.F;
    }
}

/// @brief Makes the `aylo.replication.localClient` call of @p app, as a script does.
aylo::replication::client::LocalClient localClientOf(App& app) {
    aylo::ecs::Calls& calls = app.getWorld().getCalls();
    const aylo::ecs::CallInfo* info = calls.findByName(aylo::replication::client::kLocalClientCall);
    EXPECT_NE(info, nullptr);
    if (info == nullptr) {
        return {};
    }
    std::array<std::byte, sizeof(aylo::replication::client::LocalClient)> result{};
    calls.invoke(app.getWorld(), info->id, std::span<const std::byte>{}, result);
    aylo::replication::client::LocalClient local{};
    std::memcpy(&local, result.data(), sizeof(local));
    return local;
}

/// @brief Registers the game's components and input actions in an App.
void setUpGame(App& app, const bool withSecret = false) {
    app.addPlugin(TimePlugin::manual(kFrame, FixedTimestep{kTickRate, 5}));
    app.addPlugin(ReplicationPlugin{});
    auto& components = app.getWorld().getComponents();
    static_cast<void>(
        components.registerType<Position>({.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
    if (withSecret) {
        static_cast<void>(components.registerType<Secret>(
            {.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
    }
    auto layout = app.getWorld().getResources().getMut<InputLayout>();
    layout->add("fire", InputKind::kButton);
    layout->add("move", InputKind::kAxis);
}

/// @brief A server App, started, whose port a client connects to.
class Server {
  public:
    Server() : _app{std::make_unique<App>(RunMode::kServer)} {
        setUpGame(*_app);
        _app->addPlugin(ServerPlugin{makeNetworkSettings(), {.tickRate = kTickRate, .snapshotRate = kTickRate}});
        _app->addSystem(Phase::kFixedUpdate, "game.spawnShips", &spawnShips);
        _app->addSystem(Phase::kPostUpdate, "game.moveShips", &moveShips)
            .before(std::string{ServerPlugin::kSnapshotSystem});
        _app->runFrame();  // Startup: opens the socket
    }

    [[nodiscard]] App& app() { return *_app; }

    [[nodiscard]] ServerState& state() { return *_app->getWorld().getResources().getMut<ServerState>(); }

    [[nodiscard]] network::io::Address address() {
        return network::io::Address::resolve("127.0.0.1", state().netServer().localPort()).value();
    }

  private:
    std::unique_ptr<App> _app;
};

/// @brief A client App in a given mode, with the game's components and the plugin.
class Client {
  public:
    Client(const network::io::Address& server, const RunMode mode = RunMode::kClient, const bool withSecret = false)
        : _app{std::make_unique<App>(mode)} {
        setUpGame(*_app, withSecret);
        _app->addPlugin(ClientPlugin{server, makeNetworkSettings()});
    }

    [[nodiscard]] App& app() { return *_app; }

    [[nodiscard]] ClientState& state() { return *_app->getWorld().getResources().getMut<ClientState>(); }

  private:
    std::unique_ptr<App> _app;
};

/// @brief Runs frames of both apps until `done` or two seconds pass.
bool runUntil(Server& server, Client& client, const std::function<bool()>& done) {
    const auto deadline = network::Clock::now() + 2s;
    while (network::Clock::now() < deadline) {
        server.app().runFrame();
        client.app().runFrame();
        if (done()) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }
    return false;
}
}  // namespace

TEST(ClientPlugin, IsNamed) {
    const auto address = network::io::Address::resolve("127.0.0.1", 9).value();
    EXPECT_EQ((ClientPlugin{address, network::NetworkSettings{}}.getName()), "aylo.replication.client");
    EXPECT_EQ(ClientPlugin::kName, "aylo.replication.client");
    EXPECT_EQ(ClientPlugin::kSendInputSystem, "net.sendInput");
}

TEST(ClientPlugin, AddsTheReplicationPluginAndTheState) {
    Server server;
    Client client{server.address()};
    EXPECT_TRUE(client.app().hasPlugin("aylo.replication"));
    EXPECT_TRUE(client.app().hasPlugin("aylo.replication.client"));
    EXPECT_TRUE(client.app().getWorld().getResources().has<ClientState>());
}

TEST(ClientPlugin, RunsOnlyInClientAndStandaloneModes) {
    Server server;
    for (const RunMode mode : {RunMode::kServer, RunMode::kClient, RunMode::kStandalone}) {
        Client client{server.address(), mode};
        client.app().runFrame();
        client.app().runFrame();
        const bool runs = mode != RunMode::kServer;
        EXPECT_EQ(client.state().connectionStatus() != ConnectionStatus::kIdle, runs);
    }
}

TEST(ClientPlugin, SendsReadyOnceTheManifestIsAccepted) {
    Server server;
    Client client{server.address()};
    ASSERT_TRUE(runUntil(server, client, [&] { return server.state().clientCount() == 1; }));
    EXPECT_EQ(client.state().manifestStatus(), ManifestStatus::kAccepted);
    EXPECT_EQ(client.state().connectionStatus(), ConnectionStatus::kConnected);
    EXPECT_EQ(client.state().serverSettings().tickRate, kTickRate);
}

TEST(ClientPlugin, DisconnectsAfterAMismatch) {
    Server server;
    Client client{server.address(), RunMode::kClient, true};
    ASSERT_TRUE(runUntil(server, client, [&] { return client.state().manifestStatus() != ManifestStatus::kWaiting; }));
    EXPECT_EQ(client.state().manifestStatus(), ManifestStatus::kMismatch);
    EXPECT_NE(client.state().mismatch().find("test.server.Secret"), std::string_view::npos);
    EXPECT_EQ(client.state().netClient().core().state(), network::client::ClientState::kClosed);
    for (int frame = 0; frame < 20; ++frame) {
        server.app().runFrame();
        std::this_thread::sleep_for(1ms);
    }
    EXPECT_EQ(server.state().clientCount(), 0U);
}

TEST(ClientPlugin, MirrorsTheServersEntitiesAfterThePreUpdatePhase) {
    Server server;
    Client client{server.address()};
    auto& world = client.app().getWorld();
    ASSERT_TRUE(runUntil(server, client, [&] { return client.state().lastAppliedTick() > 0; }));
    const auto player = client.state().localPlayer();
    ASSERT_TRUE(player);
    EXPECT_EQ(world.getComponents().get<Owner>(*player)->client, client.state().clientId().value());
    ASSERT_TRUE(runUntil(server, client, [&] { return world.getComponents().get<Position>(*player)->x > 3.F; }));
    EXPECT_EQ(world.getComponents().get<NetworkId>(*player)->value, 1U);
    EXPECT_EQ(client.state().find(1), *player);
}

TEST(ClientPlugin, GivesScriptsTheClientsIdOnceConnected) {
    Server server;
    Client client{server.address()};
    EXPECT_FALSE(localClientOf(client.app()).connected);
    ASSERT_TRUE(runUntil(server, client, [&] { return client.state().clientId().has_value(); }));
    const aylo::replication::client::LocalClient local = localClientOf(client.app());
    EXPECT_TRUE(local.connected);
    EXPECT_EQ(local.client, client.state().clientId().value());
}

TEST(ClientPlugin, SendsOneInputPerFrame) {
    Server server;
    Client client{server.address()};
    client.app()
        .addSystem(Phase::kPostUpdate, "game.pressFire", &pressFire)
        .setSide(aylo::ecs::Side::kClient)
        .before(std::string{ClientPlugin::kSendInputSystem});
    ASSERT_TRUE(runUntil(server, client, [&] { return server.state().clientCount() == 1; }));
    const network::ClientId id = client.state().clientId().value();
    ASSERT_TRUE(runUntil(server, client, [&] { return server.state().lastProcessedInput(id) >= 5; }));
    EXPECT_GT(server.state().lastAppliedSnapshot(id), 0U);
    const auto ship = server.state().findEntity(1);
    ASSERT_TRUE(ship);
    const std::uint32_t buttons = server.app().getWorld().getComponents().get<PlayerInput>(*ship)->buttons;
    EXPECT_EQ(buttons, 1U);
}

namespace {
/// @brief Declares the round machine of the server in @p app, as a shared script does on both sides.
aylo::ecs::States& declareRound(App& app) {
    aylo::ecs::States& states = *app.getWorld().getResources().getMut<aylo::ecs::States>();
    states.add("game.Round", {"Warmup", "Live", "Over"}, aylo::ecs::States::Replication::kFromServer);
    return states;
}

/// @return The value of the round machine in @p app, empty before it has one.
std::string roundOf(App& app) {
    const auto value = app.getWorld().getResources().require<aylo::ecs::States>().get("game.Round");
    return value ? std::string{*value} : std::string{};
}
}  // namespace

TEST(ClientPlugin, FollowsTheStatesTheServerOwns) {
    Server server;
    aylo::ecs::States& serverStates = declareRound(server.app());
    Client client{server.address()};
    declareRound(client.app());
    client.app().runFrame();
    EXPECT_EQ(roundOf(client.app()), "");  // no value of its own: it waits for the server's

    ASSERT_TRUE(runUntil(server, client, [&] { return roundOf(client.app()) == "Warmup"; }));
    serverStates.request("game.Round", "Live");
    ASSERT_TRUE(runUntil(server, client, [&] { return roundOf(client.app()) == "Live"; }));
    EXPECT_THROW(client.app().getWorld().getResources().getMut<aylo::ecs::States>()->request("game.Round", "Over"),
                 aylo::ecs::exceptions::StateException);
}

TEST(ClientPlugin, AClientThatJoinsLateGetsTheCurrentState) {
    Server server;
    aylo::ecs::States& serverStates = declareRound(server.app());
    serverStates.request("game.Round", "Over");
    server.app().runFrame();
    server.app().runFrame();
    ASSERT_EQ(roundOf(server.app()), "Over");

    Client client{server.address()};
    declareRound(client.app());
    ASSERT_TRUE(runUntil(server, client, [&] { return roundOf(client.app()) == "Over"; }));
}

TEST(ClientPlugin, DropsAStateItDoesNotKnow) {
    Server server;
    declareRound(server.app());
    Client client{server.address()};  // declares no round machine
    ASSERT_TRUE(runUntil(server, client, [&] { return client.state().droppedCount() > 0; }));
    EXPECT_FALSE(client.app().getWorld().getResources().require<aylo::ecs::States>().has("game.Round"));
}
