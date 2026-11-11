/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ServerPluginTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "ServerFixtures.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/exceptions/ScheduleException.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/system/EventReader.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/Time.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/network/NetworkSettings.hpp"
#include "aylo/network/Types.hpp"
#include "aylo/network/admin/AdminClient.hpp"
#include "aylo/network/admin/AdminProtocol.hpp"
#include "aylo/network/admin/WorldProtocol.hpp"
#include "aylo/network/client/ClientEvent.hpp"
#include "aylo/network/client/NetClient.hpp"
#include "aylo/network/exceptions/NetworkExceptions.hpp"
#include "aylo/network/io/Address.hpp"
#include "aylo/network/io/UdpSocket.hpp"
#include "aylo/network/server/NetServer.hpp"
#include "aylo/replication/MessageType.hpp"
#include "aylo/replication/ReplicationPlugin.hpp"
#include "aylo/replication/ReplicationSettings.hpp"
#include "aylo/replication/component/NetworkId.hpp"
#include "aylo/replication/component/Owner.hpp"
#include "aylo/replication/component/PlayerInput.hpp"
#include "aylo/replication/event/ClientConnected.hpp"
#include "aylo/replication/event/ClientDisconnected.hpp"
#include "aylo/replication/exceptions/ReplicationExceptions.hpp"
#include "aylo/replication/input/InputKind.hpp"
#include "aylo/replication/input/InputLayout.hpp"
#include "aylo/replication/input/InputMessage.hpp"
#include "aylo/replication/server/ServerPlugin.hpp"
#include "aylo/replication/server/ServerState.hpp"
#include "aylo/replication/snapshot/Snapshot.hpp"

namespace aylo::test::server_plugin {
/// @brief What the game systems of the tests record.
struct Log {
    std::uint32_t connected;     ///< ClientConnected events seen.
    std::uint32_t disconnected;  ///< ClientDisconnected events seen.
    std::uint32_t lastClient;    ///< The client of the last event.
    std::uint32_t buttons;       ///< The buttons a player's PlayerInput had when the game looked at it.
    std::uint32_t pending;       ///< Entities the spawner has to spawn.
};
AYLO_ECS_RESOURCE(Log, "test.serverPlugin.Log", connected, disconnected, lastClient, buttons, pending);
}  // namespace aylo::test::server_plugin

namespace {
namespace snapshot = aylo::replication::snapshot;
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
using aylo::ecs::Res;
using aylo::ecs::ResMut;
using aylo::ecs::RunMode;
using aylo::ecs::TimePlugin;
using aylo::replication::ReplicationPlugin;
using aylo::replication::ReplicationSettings;
using aylo::replication::component::NetworkId;
using aylo::replication::component::Owner;
using aylo::replication::component::PlayerInput;
using aylo::replication::event::ClientConnected;
using aylo::replication::event::ClientDisconnected;
using aylo::replication::exceptions::ReplicationException;
using aylo::replication::input::InputKind;
using aylo::replication::input::InputLayout;
using aylo::replication::server::ServerPlugin;
using aylo::replication::server::ServerState;
using aylo::test::server::Follow;
using aylo::test::server::Position;
using aylo::test::server_plugin::Log;

/// One frame is one fixed tick: 50 Hz, 20 ms per frame.
constexpr std::uint8_t kTickRate = 50;
constexpr std::chrono::milliseconds kFrame{20};

network::NetworkSettings makeNetworkSettings() {
    network::NetworkSettings settings;
    settings.port = 0;
    return settings;
}

ReplicationSettings makeSettings(const std::uint8_t snapshotRate = kTickRate) {
    return {.tickRate = kTickRate, .snapshotRate = snapshotRate};
}

/// @brief Game: spawns a ship for each client that joins.
void spawnShips(EventReader<ClientConnected> joined, Commands& commands, ResMut<Log> log) {
    for (const ClientConnected& event : joined) {
        commands.spawn().add(Owner{.client = event.client}).add(PlayerInput{}).add(Position{});
        ++log->connected;
        log->lastClient = event.client;
    }
}

/// @brief Game: despawns nothing, only counts who left.
void countDepartures(EventReader<ClientDisconnected> left, ResMut<Log> log) {
    for (const ClientDisconnected& event : left) {
        ++log->disconnected;
        log->lastClient = event.client;
    }
}

/// @brief Game: spawns the entities the test asked for.
void spawnPending(Commands& commands, ResMut<Log> log) {
    for (; log->pending > 0; --log->pending) {
        commands.spawn().add(Position{});
    }
}

/// @brief Game: looks at the input of the players.
void lookAtInputs(Query<const PlayerInput> players, ResMut<Log> log) {
    for (auto [entity, input] : players) {
        log->buttons = input.buttons;
    }
}

/// @brief Game: moves players in PostUpdate (writes a replicated component).
void writePositions(Query<Position> positions) {
    for (auto [entity, position] : positions) {
        position->x += 1.F;
    }
}

/// @brief A server App in a given mode, with the game's components, a clock of one tick per frame, and the plugin.
class Server {
  public:
    explicit Server(const RunMode mode = RunMode::kServer, const ReplicationSettings& settings = makeSettings())
        : _app{std::make_unique<App>(mode)} {
        _app->addPlugin(TimePlugin::manual(kFrame, FixedTimestep{kTickRate, 5}));
        _app->addPlugin(ReplicationPlugin{});
        auto& components = _app->getWorld().getComponents();
        static_cast<void>(components.registerType<Position>(
            {.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
        static_cast<void>(components.registerType<Follow>(
            {.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
        auto layout = _app->getWorld().getResources().getMut<InputLayout>();
        layout->add("fire", InputKind::kButton);
        layout->add("move", InputKind::kAxis);
        _app->insertResource(Log{});
        _app->addPlugin(ServerPlugin{makeNetworkSettings(), settings});
    }

    [[nodiscard]] App& app() { return *_app; }

    [[nodiscard]] aylo::ecs::World& world() { return _app->getWorld(); }

    [[nodiscard]] ServerState& state() { return *_app->getWorld().getResources().getMut<ServerState>(); }

    [[nodiscard]] Log& log() { return *_app->getWorld().getResources().getMut<Log>(); }

    /// @return The layout the clients share.
    [[nodiscard]] const InputLayout& layout() { return *_app->getWorld().getResources().get<InputLayout>(); }

    /// @brief Adds the game systems that react to clients, after the plugin.
    void addGame() {
        _app->addSystem(Phase::kFixedUpdate, "game.1spawnShips", &spawnShips);
        _app->addSystem(Phase::kFixedUpdate, "game.2countDepartures", &countDepartures).after("game.1spawnShips");
        _app->addSystem(Phase::kFixedUpdate, "game.3spawnPending", &spawnPending).after("game.2countDepartures");
        _app->addSystem(Phase::kFixedUpdate, "game.4lookAtInputs", &lookAtInputs)
            .after(std::string{ServerPlugin::kApplyInputsSystem})
            .after("game.3spawnPending");
    }

  private:
    std::unique_ptr<App> _app;  ///< Not movable, so held by pointer.
};

/// @brief A client on loopback, driven by hand.
class Remote {
  public:
    explicit Remote(const std::uint16_t port)
        : _client{network::io::Address::resolve("127.0.0.1", port).value(), makeNetworkSettings()} {}

    /// @brief Polls and keeps the events.
    void poll() {
        _client.poll(network::Clock::now());
        for (network::client::ClientEvent& event : _client.takeEvents()) {
            if (auto* message = std::get_if<network::client::ServerMessage>(&event)) {
                _messages.push_back(std::move(message->payload));
            }
        }
    }

    [[nodiscard]] network::client::NetClient& client() { return _client; }

    [[nodiscard]] std::vector<network::Bytes>& messages() { return _messages; }

  private:
    network::client::NetClient _client;
    std::vector<network::Bytes> _messages;  ///< The messages of the server, in order.
};

/// @brief Runs frames, polling the remote client, until `done` or two seconds pass.
bool runUntil(Server& server, Remote* remote, const std::function<bool()>& done) {
    const auto deadline = network::Clock::now() + 2s;
    while (network::Clock::now() < deadline) {
        server.app().runFrame();
        if (remote != nullptr) {
            remote->poll();
        }
        if (done()) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }
    return false;
}

/// @brief Connects a client: runs frames until the server knows it.
bool connect(Server& server, Remote& remote) {
    server.app().runFrame();  // Startup: opens the socket
    remote.client().connect(network::Clock::now());
    if (!runUntil(server, &remote,
                  [&] { return remote.client().core().state() == network::client::ClientState::kConnected; })) {
        return false;
    }
    remote.client().sendReady();
    return runUntil(server, &remote, [&] { return server.state().clientCount() == 1; });
}

/// @brief Opens the server and gives the port a client connects to.
std::uint16_t open(Server& server) {
    server.app().runFrame();
    return server.state().netServer().localPort();
}

/// @return The last snapshot a remote received, decoded.
std::optional<snapshot::Snapshot> lastSnapshot(Server& server, Remote& remote) {
    const std::vector<std::size_t> sizes = snapshot::computeComponentSizes(server.state().manifest(), server.world());
    for (auto message = remote.messages().rbegin(); message != remote.messages().rend(); ++message) {
        if (!message->empty() &&
            message->front() == static_cast<std::byte>(aylo::replication::MessageType::kSnapshot)) {
            return snapshot::decode(*message, sizes);
        }
    }
    return std::nullopt;
}

/// @return The NetworkId an entity was given, or 0.
std::uint32_t networkIdOf(Server& server, const aylo::ecs::Entity entity) {
    const NetworkId* id = server.world().getComponents().get<NetworkId>(entity);
    return id == nullptr ? 0 : id->value;
}
}  // namespace

TEST(ServerPlugin, IsNamed) {
    EXPECT_EQ((ServerPlugin{network::NetworkSettings{}, ReplicationSettings{}}.getName()), "rtype.replication.server");
    EXPECT_EQ(ServerPlugin::kName, "rtype.replication.server");
    EXPECT_EQ(ServerPlugin::kReceiveSystem, "net.receive");
    EXPECT_EQ(ServerPlugin::kApplyInputsSystem, "net.applyInputs");
    EXPECT_EQ(ServerPlugin::kAssignIdsSystem, "net.assignIds");
    EXPECT_EQ(ServerPlugin::kSnapshotSystem, "net.snapshot");
}

TEST(ServerPlugin, AddsTheReplicationPluginAndTheState) {
    Server server;
    EXPECT_TRUE(server.app().hasPlugin("rtype.replication"));
    EXPECT_TRUE(server.app().hasPlugin("rtype.replication.server"));
    EXPECT_TRUE(server.world().getResources().has<ServerState>());
}

TEST(ServerPlugin, AddingItTwiceDoesNothingTheSecondTime) {
    Server server;
    EXPECT_NO_THROW(server.app().addPlugin(ServerPlugin{makeNetworkSettings(), makeSettings()}));
}

TEST(ServerPlugin, OpensTheServerInServerMode) {
    Server server{RunMode::kServer};
    server.app().runFrame();
    EXPECT_TRUE(server.state().isStarted());
    EXPECT_NE(server.state().netServer().localPort(), 0U);
    EXPECT_TRUE(server.layout().isFrozen());
}

TEST(ServerPlugin, OpensTheAdminChannelOnlyWhenAskedAndAnswersIt) {
    {
        Server server{RunMode::kServer};
        server.app().runFrame();
        EXPECT_EQ(server.state().adminPort(), 0U);  // off by default
    }
    App app{RunMode::kServer};
    app.addPlugin(TimePlugin::manual(kFrame, FixedTimestep{kTickRate, 5}));
    network::NetworkSettings settings = makeNetworkSettings();
    std::uint16_t adminPort = 0;
    // A free port for the admin channel: ask the system for one, then release it.
    {
        network::io::UdpSocket probe{0, true};
        adminPort = probe.localPort();
    }
    ServerPlugin plugin{settings, makeSettings()};
    plugin.setAdmin({.port = adminPort, .token = "tok"});
    app.addPlugin(std::move(plugin));
    app.runFrame();
    auto state = app.getWorld().getResources().getMut<ServerState>();
    ASSERT_NE(state->adminPort(), 0U);

    network::admin::AdminClient tool{state->adminPort(), "tok"};
    const std::uint32_t id = tool.send(network::admin::StatusRequest{});
    bool answered = false;
    const auto deadline = network::Clock::now() + std::chrono::seconds{2};
    while (!answered && network::Clock::now() < deadline) {
        app.runFrame();
        for (const auto& reply : tool.poll()) {
            answered = answered || (reply.id == id && std::holds_alternative<network::admin::StatusReply>(reply.reply));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    EXPECT_TRUE(answered);
}

TEST(ServerPlugin, TheAdminChannelEditsTheWorldBetweenFramesAndPausesIt) {
    App app{RunMode::kServer};
    app.addPlugin(TimePlugin::manual(kFrame, FixedTimestep{kTickRate, 5}));
    std::uint16_t adminPort = 0;
    {
        network::io::UdpSocket probe{0, true};
        adminPort = probe.localPort();
    }
    ServerPlugin plugin{makeNetworkSettings(), makeSettings()};
    plugin.setAdmin({.port = adminPort, .token = "tok"});
    app.addPlugin(std::move(plugin));
    app.runFrame();
    auto state = app.getWorld().getResources().getMut<ServerState>();
    ASSERT_NE(state->adminPort(), 0U);
    network::admin::AdminClient tool{state->adminPort(), "tok"};

    // Sends a world request and runs frames until its reply comes.
    const auto ask = [&](const network::admin::WorldRequest& request) -> std::optional<network::admin::Reply> {
        const std::uint32_t id = tool.send(network::admin::encodeWorldRequest(request));
        const auto deadline = network::Clock::now() + std::chrono::seconds{2};
        while (network::Clock::now() < deadline) {
            app.runFrame();
            for (auto& reply : tool.poll()) {
                if (reply.id == id) {
                    return std::move(reply.reply);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        return std::nullopt;
    };

    const auto spawned = ask(network::admin::SpawnRequest{.name = "Probe"});
    ASSERT_TRUE(spawned.has_value());
    const auto reply = network::admin::decodeWorldReply(std::get<network::admin::ExtensionReply>(*spawned));
    ASSERT_TRUE(reply.has_value());
    const std::uint64_t bits = std::get<network::admin::SpawnReply>(*reply).entity;
    app.runFrame();  // the spawn is applied after the phase that answered it
    const aylo::ecs::Entity entity{static_cast<std::uint32_t>(bits), static_cast<std::uint32_t>(bits >> 32U)};
    ASSERT_TRUE(app.getWorld().isAlive(entity));

    ASSERT_TRUE(ask(network::admin::RunRequest{.action = network::admin::RunAction::kPause}).has_value());
    EXPECT_TRUE(app.getSchedules().isPaused());
    ASSERT_TRUE(ask(network::admin::RunRequest{.action = network::admin::RunAction::kResume}).has_value());
    EXPECT_FALSE(app.getSchedules().isPaused());

    ASSERT_TRUE(ask(network::admin::DespawnRequest{.entity = bits}).has_value());
    app.runFrame();
    EXPECT_FALSE(app.getWorld().isAlive(entity));
}

TEST(ServerPlugin, OpensTheServerInStandaloneMode) {
    Server server{RunMode::kStandalone};
    server.app().runFrame();
    EXPECT_TRUE(server.state().isStarted());
}

TEST(ServerPlugin, RunsNothingInClientMode) {
    Server server{RunMode::kClient};
    server.app().runFrame();
    server.app().runFrame();
    EXPECT_FALSE(server.state().isStarted());
    EXPECT_FALSE(server.layout().isFrozen());
}

TEST(ServerPlugin, StartFailsWhenTheSnapshotRateDoesNotDivideTheTickRate) {
    Server server{RunMode::kServer, makeSettings(7)};
    EXPECT_THROW(server.app().runFrame(), ReplicationException);
}

TEST(ServerPlugin, StartFailsWhenThePortIsTaken) {
    network::server::NetServer taken{makeNetworkSettings()};
    network::NetworkSettings settings;
    settings.port = taken.localPort();
    App app;
    app.addPlugin(TimePlugin::manual(kFrame, FixedTimestep{kTickRate, 5}));
    app.addPlugin(ServerPlugin{settings, makeSettings()});
    EXPECT_THROW(app.runFrame(), network::exceptions::SocketException);
}

TEST(ServerPlugin, StartNamesAReplicatedComponentRegisteredAfterThePlugin) {
    Server server;
    aylo::ecs::ComponentInfo info;
    info.type.name = "test.serverPlugin.Late";
    info.type.size = sizeof(float);
    info.type.alignment = alignof(float);
    info.type.presence = Presence::kBoth;
    info.type.replication = ReplicationMode::kEveryChange;
    info.type.fields.push_back({.name = "value", .type = aylo::ecs::FieldType::kF32, .offset = 0, .count = 1});
    ASSERT_TRUE(server.world().getComponents().registerType(info));

    try {
        server.app().runFrame();
        FAIL() << "net.start should refuse a component registered after the plugin";
    } catch (const ReplicationException& error) {
        EXPECT_NE(std::string{error.what()}.find("test.serverPlugin.Late"), std::string::npos) << error.what();
    }
}

TEST(ServerPlugin, AGameSystemWritingAReplicatedComponentMustBeOrderedAgainstTheSnapshot) {
    Server server;
    server.app().addSystem(Phase::kPostUpdate, "game.writePositions", &writePositions);
    try {
        server.app().runFrame();
        FAIL() << "the schedule should not build";
    } catch (const aylo::ecs::exceptions::ScheduleException& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("game.writePositions"), std::string::npos) << message;
        EXPECT_NE(message.find("net.snapshot"), std::string::npos) << message;
        EXPECT_EQ(message.find("net.assignIds"), std::string::npos) << message;
    }
}

TEST(ServerPlugin, OrderingAWriterAgainstTheSnapshotIsEnough) {
    Server before;
    before.app()
        .addSystem(Phase::kPostUpdate, "game.writePositions", &writePositions)
        .before(std::string{ServerPlugin::kSnapshotSystem});
    EXPECT_NO_THROW(before.app().runFrame());

    Server after;
    after.app()
        .addSystem(Phase::kPostUpdate, "game.writePositions", &writePositions)
        .after(std::string{ServerPlugin::kSnapshotSystem});
    EXPECT_NO_THROW(after.app().runFrame());
}

TEST(ServerPlugin, GivesIncreasingIdsInTheFrameTheEntityWasSpawned) {
    Server server;
    server.addGame();
    server.app().runFrame();

    server.log().pending = 2;
    server.app().runFrame();

    std::vector<std::uint32_t> ids;
    for (std::uint32_t networkId = 1; networkId <= 2; ++networkId) {
        const auto entity = server.state().findEntity(networkId);
        ASSERT_TRUE(entity) << networkId;
        ids.push_back(networkIdOf(server, *entity));
    }
    EXPECT_EQ(ids, (std::vector<std::uint32_t>{1, 2}));
    EXPECT_EQ(server.state().entityCount(), 2U);
}

TEST(ServerPlugin, AnEntityWithSeveralReplicatedComponentsGetsOneId) {
    Server server;
    server.app().runFrame();
    const aylo::ecs::Entity target = server.world().spawn(Position{});
    const aylo::ecs::Entity both = server.world().spawn(Position{}, Follow{.target = target});

    server.app().runFrame();

    EXPECT_EQ(server.state().entityCount(), 2U);
    EXPECT_EQ(networkIdOf(server, target), 1U);
    EXPECT_EQ(networkIdOf(server, both), 2U);
    server.app().runFrame();
    EXPECT_EQ(server.state().entityCount(), 2U);
}

TEST(ServerPlugin, IgnoresEntitiesWithoutReplicatedComponents) {
    Server server;
    server.app().runFrame();
    static_cast<void>(server.world().spawn());
    server.app().runFrame();
    EXPECT_EQ(server.state().entityCount(), 0U);
}

TEST(ServerPlugin, NeverReusesAnId) {
    Server server;
    server.addGame();
    server.app().runFrame();
    server.log().pending = 1;
    server.app().runFrame();
    const aylo::ecs::Entity first = *server.state().findEntity(1);
    server.world().despawn(first);
    server.app().runFrame();  // forgets it, at the snapshot
    server.log().pending = 1;
    server.app().runFrame();

    EXPECT_FALSE(server.state().findEntity(1));
    EXPECT_TRUE(server.state().findEntity(2));
    EXPECT_EQ(server.state().entityCount(), 1U);
}

TEST(ServerPlugin, SendsSnapshotsEveryTickRateOverSnapshotRateTicks) {
    Server server{RunMode::kServer, makeSettings(25)};  // every 2 ticks
    std::vector<std::uint32_t> ticks;
    for (int frame = 0; frame < 8; ++frame) {
        server.app().runFrame();
        ticks.push_back(server.state().serverTick());
    }
    // One tick per frame, a snapshot every second tick.
    EXPECT_EQ(ticks, (std::vector<std::uint32_t>{0, 2, 2, 4, 4, 6, 6, 8}));
}

TEST(ServerPlugin, SendsASnapshotEveryFrameAtTheTickRate) {
    Server server;
    std::vector<std::uint32_t> ticks;
    for (int frame = 0; frame < 4; ++frame) {
        server.app().runFrame();
        ticks.push_back(server.state().serverTick());
    }
    EXPECT_EQ(ticks, (std::vector<std::uint32_t>{1, 2, 3, 4}));
}

TEST(ServerPlugin, AnnouncesClientsAndSendsThemSnapshots) {
    Server server;
    server.addGame();
    const std::uint16_t port = open(server);
    Remote remote{port};
    ASSERT_TRUE(connect(server, remote));

    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.log().connected == 1; }));
    EXPECT_EQ(server.log().lastClient, 1U);
    EXPECT_EQ(server.log().disconnected, 0U);

    // The ship spawned by the game is in the next snapshots, with its owner.
    ASSERT_TRUE(runUntil(server, &remote, [&] {
        const std::optional<snapshot::Snapshot> last = lastSnapshot(server, remote);
        return last && !last->spawns.empty();
    }));
    const snapshot::Snapshot last = *lastSnapshot(server, remote);
    EXPECT_EQ(last.flags, snapshot::kComplete);
    ASSERT_EQ(last.spawns.size(), 1U);
    EXPECT_EQ(last.spawns[0].networkId, 1U);
    EXPECT_EQ(last.spawns[0].components.size(), 2U);  // Owner and Position
    EXPECT_EQ(last.updates.size(), 0U);
}

TEST(ServerPlugin, AnnouncesAClientThatLeaves) {
    Server server;
    server.addGame();
    const std::uint16_t port = open(server);
    Remote remote{port};
    ASSERT_TRUE(connect(server, remote));
    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.log().connected == 1; }));

    remote.client().disconnect(network::Clock::now());

    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.log().disconnected == 1; }));
    EXPECT_EQ(server.log().lastClient, 1U);
    EXPECT_EQ(server.state().clientCount(), 0U);
}

TEST(ServerPlugin, DeadEntitiesDisappearFromTheNextSnapshot) {
    Server server;
    server.addGame();
    const std::uint16_t port = open(server);
    Remote remote{port};
    ASSERT_TRUE(connect(server, remote));
    ASSERT_TRUE(runUntil(server, &remote, [&] {
        const std::optional<snapshot::Snapshot> last = lastSnapshot(server, remote);
        return last && !last->spawns.empty();
    }));

    server.world().despawn(*server.state().findEntity(1));
    const std::size_t received = remote.messages().size();

    ASSERT_TRUE(runUntil(server, &remote, [&] { return remote.messages().size() > received + 1; }));
    const snapshot::Snapshot last = *lastSnapshot(server, remote);
    EXPECT_TRUE(last.spawns.empty());
    EXPECT_EQ(last.flags, snapshot::kComplete);
    EXPECT_EQ(server.state().entityCount(), 0U);
}

TEST(ServerPlugin, AGameSystemOrderedAfterApplyInputsSeesTheInput) {
    Server server;
    server.addGame();
    const std::uint16_t port = open(server);
    Remote remote{port};
    ASSERT_TRUE(connect(server, remote));
    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.log().connected == 1; }));
    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.world().getEntityCount() == 1; }));
    ASSERT_EQ(server.log().buttons, 0U);

    const network::Bytes message = aylo::replication::input::encodeInput(
        {.lastAppliedSnapshot = 0, .inputs = {{.sequence = 1, .buttons = 0b1, .axes = {}}}}, server.layout());
    ASSERT_TRUE(remote.client().send(network::Channel::kUnreliable, message));

    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.log().buttons == 0b1; }));
    EXPECT_EQ(server.state().lastProcessedInput(1), 1U);
}

TEST(ServerPlugin, TheSnapshotCarriesTheLastProcessedInputOfTheClient) {
    Server server;
    server.addGame();
    const std::uint16_t port = open(server);
    Remote remote{port};
    ASSERT_TRUE(connect(server, remote));
    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.world().getEntityCount() == 1; }));

    const network::Bytes message = aylo::replication::input::encodeInput(
        {.lastAppliedSnapshot = 0, .inputs = {{.sequence = 3, .buttons = 0b1, .axes = {}}}}, server.layout());
    ASSERT_TRUE(remote.client().send(network::Channel::kUnreliable, message));

    ASSERT_TRUE(runUntil(server, &remote, [&] {
        const std::optional<snapshot::Snapshot> last = lastSnapshot(server, remote);
        return last && last->lastProcessedInput == 3;
    }));
}
