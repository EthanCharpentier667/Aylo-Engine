/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ReplicatedCapsuleTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/entity/Name.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/query/QueryState.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/ecs/world/World.hpp"
#include "aylo/network/NetworkSettings.hpp"
#include "aylo/network/io/Address.hpp"
#include "aylo/replication/ReplicationPlugin.hpp"
#include "aylo/replication/client/ClientPlugin.hpp"
#include "aylo/replication/client/ClientState.hpp"
#include "aylo/replication/server/ServerPlugin.hpp"
#include "aylo/replication/server/ServerState.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "plugins/capsule/SpawnedCapsule.hpp"

namespace aylo::test::replicated_capsule {
/// @brief Replicated: what the server decides.
struct Ship {
    float x{0.0F};
};
AYLO_ECS_COMPONENT(Ship, "test.rc.Ship", x);

/// @brief Not replicated: a look, which each side has from the capsule.
struct Look {
    std::int32_t layer{0};
};
AYLO_ECS_COMPONENT(Look, "test.rc.Look", layer);
}  // namespace aylo::test::replicated_capsule

namespace {
using namespace std::chrono_literals;  // NOLINT(google-build-using-namespace): only the literals
namespace capsule = aylo::plugins::capsule;
namespace rc = aylo::test::replicated_capsule;
using aylo::ecs::App;
using aylo::ecs::RunMode;

constexpr std::uint8_t kTickRate = 50;

/// @brief Registers the components and the capsules both sides have:
/// - "actors/ship.actor": a root (Ship 1, Look 7), a client "Flame" (Look 9) with a "Spark" (Look 10), a server
///   "Hitbox" (Look 3) and a "Shadow" on both sides (Look 4);
/// - "actors/brain.actor": a root with a Look only, which never reaches the clients;
/// - "scenes/arena.scene": a root with a Ship, which both sides load.
void setUp(App& app) {
    app.addPlugin(aylo::ecs::TimePlugin::manual(20ms, aylo::ecs::FixedTimestep{kTickRate, 5}));
    app.addPlugin(aylo::replication::ReplicationPlugin{});
    auto& components = app.getWorld().getComponents();
    static_cast<void>(components.registerType<rc::Ship>(
        {.presence = aylo::ecs::Presence::kBoth, .replication = aylo::ecs::ReplicationMode::kEveryChange}));
    static_cast<void>(components.registerType<rc::Look>());
    app.addPlugin(capsule::CapsulePlugin{});
    auto& capsules = *app.getWorld().getResources().getMut<capsule::Capsules>();

    capsule::Capsule ship{"actors/ship.actor"};
    ship.editNode(0).values.set(components, rc::Ship{.x = 1.0F});
    ship.editNode(0).values.set(components, rc::Look{.layer = 7});
    const auto child = [&](const char* name, std::uint32_t parent, aylo::ecs::Side side, std::int32_t layer) {
        const std::uint32_t node = ship.addNode(name, parent);
        ship.editNode(node).side = side;
        ship.editNode(node).values.set(components, rc::Look{.layer = layer});
        return node;
    };
    const std::uint32_t flame = child("Flame", 0, aylo::ecs::Side::kClient, 9);
    static_cast<void>(child("Spark", flame, aylo::ecs::Side::kClient, 10));
    static_cast<void>(child("Hitbox", 0, aylo::ecs::Side::kServer, 3));
    static_cast<void>(child("Shadow", 0, aylo::ecs::Side::kBoth, 4));
    capsules.define(std::move(ship));

    capsule::Capsule brain{"actors/brain.actor"};
    brain.editNode(0).values.set(components, rc::Look{.layer = 1});
    capsules.define(std::move(brain));

    capsule::Capsule arena{"scenes/arena.scene"};
    arena.editNode(0).values.set(components, rc::Ship{.x = 0.0F});
    capsules.define(std::move(arena));
}

aylo::network::NetworkSettings networkSettings() {
    aylo::network::NetworkSettings settings;
    settings.port = 0;
    return settings;
}

/// @return The layers of the entities of @p app with a Look, sorted.
std::vector<std::int32_t> layersOf(App& app) {
    aylo::ecs::QueryState<const rc::Look> looks{app.getWorld().getComponents()};
    std::vector<std::int32_t> layers;
    for (auto [entity, look] : looks.iter(0)) {
        layers.push_back(look.layer);
    }
    std::ranges::sort(layers);
    return layers;
}

/// @brief Runs frames of both apps until @p done or two seconds pass.
bool runUntil(App& server, App& client, const std::function<bool()>& done) {
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline) {
        server.runFrame();
        client.runFrame();
        if (done()) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }
    return false;
}

/// @brief Records spawning the capsule at @p path in @p app, flushed.
aylo::ecs::Entity spawn(App& app, const char* path) {
    aylo::ecs::Commands commands{app.getWorld()};
    const aylo::ecs::Entity root =
        capsule::spawnCapsule(commands, *app.getWorld().getResources().get<capsule::Capsules>(),
                              capsule::CapsuleId::fromPath(path))
            .getEntity();
    commands.flush();
    return root;
}
}  // namespace

TEST(ReplicatedCapsule, AnActorTheServerSpawnsBringsItsClientPart) {
    // A client spawns the part of a server-spawned actor the server cannot send (#166): the root's look and its client
    // nodes, once; the server's own nodes are not doubled.
    App server{RunMode::kServer};
    setUp(server);
    server.addPlugin(
        aylo::replication::server::ServerPlugin{networkSettings(), {.tickRate = kTickRate, .snapshotRate = kTickRate}});
    server.runFrame();  // Startup: opens the socket
    const auto port =
        server.getWorld().getResources().getMut<aylo::replication::server::ServerState>()->netServer().localPort();
    App client{RunMode::kClient};
    setUp(client);
    client.addPlugin(aylo::replication::client::ClientPlugin{
        aylo::network::io::Address::resolve("127.0.0.1", port).value(), networkSettings()});

    const aylo::ecs::Entity onServer = spawn(server, "actors/ship.actor");
    EXPECT_NE(server.getWorld().getComponents().get<capsule::SpawnedCapsule>(onServer), nullptr);
    EXPECT_EQ(layersOf(server), (std::vector<std::int32_t>{3, 4, 7})) << "the server spawns its nodes only";

    ASSERT_TRUE(runUntil(server, client, [&] { return layersOf(client).size() >= 3; })) << layersOf(client).size();
    for (int frame = 0; frame < 10; ++frame) {  // nothing more comes, nothing is spawned twice
        server.runFrame();
        client.runFrame();
    }

    EXPECT_EQ(layersOf(client), (std::vector<std::int32_t>{7, 9, 10}))
        << "the root's look and the client nodes; the server's nodes come with their replicated components only";
    aylo::ecs::QueryState<const rc::Ship, const capsule::SpawnedCapsule, const capsule::CapsuleInstance,
                          const aylo::ecs::Name>
        roots{client.getWorld().getComponents()};
    std::vector<aylo::ecs::Entity> found;
    for (auto [entity, ship, spawned, instance, name] : roots.iter(0)) {
        found.push_back(entity);
        EXPECT_EQ(ship.x, 1.0F);
        EXPECT_EQ(instance.capsule, capsule::CapsuleId::fromPath("actors/ship.actor"));
        EXPECT_EQ(name.value.view(), "ship");
    }
    ASSERT_EQ(found.size(), 1U);
    aylo::ecs::QueryState<const rc::Look, const aylo::ecs::Parent, const aylo::ecs::Name> children{
        client.getWorld().getComponents()};
    bool flame = false;
    for (auto [entity, look, parent, name] : children.iter(0)) {
        if (name.value.view() == "Flame") {
            flame = true;
            EXPECT_EQ(parent.entity, found.front()) << "parented to the replicated root";
        }
    }
    EXPECT_TRUE(flame);
}

TEST(ReplicatedCapsule, OnlyARootThatReachesTheClientsSaysItsCapsule) {
    App server{RunMode::kServer};
    setUp(server);
    const aylo::ecs::World& world = server.getWorld();

    EXPECT_NE(world.getComponents().get<capsule::SpawnedCapsule>(spawn(server, "actors/ship.actor")), nullptr);
    EXPECT_EQ(world.getComponents().get<capsule::SpawnedCapsule>(spawn(server, "actors/brain.actor")), nullptr)
        << "no replicated component: it stays off the network";

    aylo::ecs::Commands commands{server.getWorld()};
    auto& loaded = *server.getWorld().getResources().getMut<capsule::LoadedCapsule>();
    const aylo::ecs::Entity arena = capsule::loadCapsule(commands, *world.getResources().get<capsule::Capsules>(),
                                                         loaded, capsule::CapsuleId::fromPath("scenes/arena.scene"))
                                        .getEntity();
    commands.flush();
    EXPECT_EQ(world.getComponents().get<capsule::SpawnedCapsule>(arena), nullptr) << "a level both sides load";

    App client{RunMode::kClient};
    setUp(client);
    EXPECT_EQ(client.getWorld().getComponents().get<capsule::SpawnedCapsule>(spawn(client, "actors/ship.actor")),
              nullptr)
        << "a client's own spawn is complete already";
    client.runFrame();
    EXPECT_EQ(layersOf(client), (std::vector<std::int32_t>{4, 7, 9, 10})) << "spawned once, by the client";
}

TEST(ReplicatedCapsule, ARootOfACapsuleTheClientDoesNotKnowIsReportedOnce) {
    App client{RunMode::kClient};
    setUp(client);
    aylo::ecs::Commands commands{client.getWorld()};
    const aylo::ecs::Entity root =
        commands.spawn()
            .add(capsule::SpawnedCapsule{.capsule = capsule::CapsuleId::fromPath("actors/gone.actor")})
            .getEntity();
    commands.flush();

    testing::internal::CaptureStderr();
    client.runFrame();
    client.runFrame();
    const std::string output = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(client.getWorld().isAlive(root));
    EXPECT_NE(client.getWorld().getComponents().get<capsule::CapsuleInstance>(root), nullptr);
    const auto first = output.find("has no client part");
    EXPECT_NE(first, std::string::npos) << output;
    EXPECT_EQ(output.find("has no client part", first + 1), std::string::npos) << output;
}
