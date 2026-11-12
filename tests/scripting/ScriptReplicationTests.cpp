/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptReplicationTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/String.hpp"
#include "aylo/network/NetworkSettings.hpp"
#include "aylo/network/Types.hpp"
#include "aylo/network/io/Address.hpp"
#include "aylo/replication/ReplicationPlugin.hpp"
#include "aylo/replication/client/ClientPlugin.hpp"
#include "aylo/replication/client/ClientState.hpp"
#include "aylo/replication/input/InputLayout.hpp"
#include "aylo/replication/server/ServerPlugin.hpp"
#include "aylo/replication/server/ServerState.hpp"
#include "aylo/scripting/component/ComponentDeclaration.hpp"
#include "aylo/scripting/component/ComponentLayout.hpp"
#include "aylo/scripting/component/ComponentRegistration.hpp"
#include "aylo/scripting/plugin/ScriptingPlugin.hpp"

namespace {
namespace network = aylo::network;
namespace sc = aylo::scripting::component;
using namespace std::chrono_literals;  // NOLINT(google-build-using-namespace): only the literals
using aylo::ecs::App;
using aylo::ecs::Entity;
using aylo::ecs::FieldType;
using aylo::ecs::ReplicationMode;
using aylo::ecs::RunMode;
using aylo::replication::client::ClientPlugin;
using aylo::replication::client::ClientState;
using aylo::replication::client::ManifestStatus;
using aylo::replication::server::ServerPlugin;
using aylo::replication::server::ServerState;

constexpr std::uint8_t kTickRate = 50;
constexpr std::chrono::milliseconds kFrame{20};

/// @brief What `game.Shield` declares: the layout of a script component is made from this alone.
sc::ComponentDeclaration makeShield(std::vector<sc::FieldDeclaration> fields) {
    return {.name = "game.Shield", .fields = std::move(fields), .replication = ReplicationMode::kEveryChange};
}

std::vector<sc::FieldDeclaration> shieldFields() {
    return {{.name = "hp", .type = FieldType::kU32}, {.name = "owner", .type = FieldType::kString}};
}

network::NetworkSettings makeNetworkSettings() {
    network::NetworkSettings settings;
    settings.port = 0;
    return settings;
}

/// @brief A scripts directory in the temporary directory, removed with it.
class ScriptsDirectory {
  public:
    explicit ScriptsDirectory(const std::string_view name, const std::string_view content)
        : _path{std::filesystem::temp_directory_path() / std::string{name}} {
        std::filesystem::remove_all(_path);
        std::filesystem::create_directories(_path);
        std::ofstream{_path / "main.luau"} << content;
    }

    ~ScriptsDirectory() {
        std::error_code error;
        std::filesystem::remove_all(_path, error);
    }

    ScriptsDirectory(const ScriptsDirectory& other) = delete;
    ScriptsDirectory& operator=(const ScriptsDirectory& other) = delete;
    ScriptsDirectory(ScriptsDirectory&& other) = delete;
    ScriptsDirectory& operator=(ScriptsDirectory&& other) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return _path; }

  private:
    std::filesystem::path _path;  ///< The directory.
};

/// @brief What `game.Shield` looks like in a world: its id, its fields and a zeroed value.
struct Shield {
    aylo::ecs::ComponentId id{};
    std::vector<aylo::ecs::FieldInfo> fields;
    std::vector<std::byte> defaults;
};

/// @return The Luau source of a script that declares `game.Shield` with the given fields, as the scripts write them.
std::string shieldScript(const std::string_view fields) {
    return std::format(R"(engine.ecs.component("game.Shield", {{ fields = {{ {} }}, replication = "everyChange" }}))",
                       fields);
}

constexpr std::string_view kShieldFields = R"(hp = "u32", owner = "string")";

/// @brief An App with the replication plugin and a `game.Shield` component, which a script or the C++ side declared.
struct Game {
    std::unique_ptr<App> app;
    Shield shield;

    /// @brief Declares the component from C++, the way the scripting plugin registers one.
    Game(const RunMode mode, const sc::ComponentDeclaration& declaration) : app{makeApp(mode)} {
        auto registered = sc::registerComponent(app->getWorld().getComponents(), declaration);
        EXPECT_TRUE(registered);
        findShield();
    }

    /// @brief Loads the Luau files of a directory, as a game does: the component comes from the script.
    Game(const RunMode mode, const std::filesystem::path& scripts) : app{makeApp(mode)} {
        app->addPlugin(aylo::scripting::ScriptingPlugin{
            {.directory = scripts,
             .options = {.isClient = mode == RunMode::kClient, .isServer = mode == RunMode::kServer}}});
        findShield();
    }

    [[nodiscard]] static std::unique_ptr<App> makeApp(const RunMode mode) {
        auto app = std::make_unique<App>(mode);
        app->addPlugin(aylo::ecs::TimePlugin::manual(kFrame, aylo::ecs::FixedTimestep{kTickRate, 5}));
        app->addPlugin(aylo::replication::ReplicationPlugin{});
        app->getWorld().getResources().getMut<aylo::replication::input::InputLayout>()->add(
            "fire", aylo::replication::input::InputKind::kButton);
        return app;
    }

    /// @brief Reads what the world knows of `game.Shield`, if a script or the C++ side registered it.
    void findShield() {
        auto& components = app->getWorld().getComponents();
        const auto id = components.find("game.Shield");
        if (!id) {
            return;
        }
        const auto* info = components.getRegistry().findById(*id);
        ASSERT_NE(info, nullptr);
        shield = {.id = *id, .fields = info->type.fields, .defaults = std::vector<std::byte>(info->type.size)};
    }

    /// @return The value of a field of `game.Shield` in the entity, read from its bytes.
    template <typename T>
    [[nodiscard]] T read(const Entity entity, const std::string_view field) const {
        const auto bytes = app->getWorld().getComponents().get(entity, shield.id);
        for (const auto& candidate : shield.fields) {
            if (candidate.name == field) {
                T value{};
                std::memcpy(&value, bytes.data() + candidate.offset, sizeof(T));
                return value;
            }
        }
        return {};
    }

    /// @brief Writes a field of `game.Shield` in the entity's bytes.
    template <typename T>
    void write(const Entity entity, const std::string_view field, const T& value) {
        const auto bytes = app->getWorld().getComponents().getMut(entity, shield.id);
        for (const auto& candidate : shield.fields) {
            if (candidate.name == field) {
                std::memcpy(bytes.data() + candidate.offset, &value, sizeof(T));
            }
        }
    }
};

/// @brief Runs frames of both apps until `done` or two seconds pass.
bool runUntil(Game& server, Game& client, const std::function<bool()>& done) {
    const auto deadline = network::Clock::now() + 2s;
    while (network::Clock::now() < deadline) {
        server.app->runFrame();
        client.app->runFrame();
        if (done()) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }
    return false;
}

/// @brief A server and a client that load the scripts they are given.
struct Pair {
    Game server;
    Game client;

    Pair(const sc::ComponentDeclaration& serverShield, const sc::ComponentDeclaration& clientShield)
        : Pair{Game{RunMode::kServer, serverShield}, Game{RunMode::kClient, clientShield}} {}

    Pair(Game serverGame, Game clientGame) : server{std::move(serverGame)}, client{std::move(clientGame)} {
        server.app->addPlugin(ServerPlugin{makeNetworkSettings(), {.tickRate = kTickRate, .snapshotRate = kTickRate}});
        server.app->runFrame();  // Startup: opens the socket
        const auto address = network::io::Address::resolve("127.0.0.1", serverState().netServer().localPort());
        client.app->addPlugin(ClientPlugin{address.value(), makeNetworkSettings()});
    }

    [[nodiscard]] ServerState& serverState() { return *server.app->getWorld().getResources().getMut<ServerState>(); }

    [[nodiscard]] ClientState& clientState() { return *client.app->getWorld().getResources().getMut<ClientState>(); }

    /// @brief Spawns a shield on the server.
    Entity spawnShield(const std::uint32_t hp, const aylo::ecs::String& owner) {
        const Entity shield = server.app->getWorld().spawn();
        EXPECT_TRUE(server.app->getWorld().getComponents().add(shield, server.shield.id, server.shield.defaults));
        server.write(shield, "hp", hp);
        server.write(shield, "owner", owner);
        return shield;
    }
};
}  // namespace

TEST(ScriptReplication, ListsAScriptComponentInTheManifestOfTheServer) {
    Pair pair{makeShield(shieldFields()), makeShield(shieldFields())};

    const auto& manifest = pair.serverState().manifest();

    bool listed = false;
    for (const auto& entry : manifest.componentEntries()) {
        listed = listed || entry.name == "game.Shield";
    }
    EXPECT_TRUE(listed);
}

TEST(ScriptReplication, SendsTheValueOfAScriptComponentToTheClient) {
    Pair pair{makeShield(shieldFields()), makeShield(shieldFields())};
    const Entity shield = pair.spawnShield(100, "Ana");

    ASSERT_TRUE(runUntil(pair.server, pair.client, [&] { return pair.clientState().find(1) != Entity{}; }));

    EXPECT_EQ(pair.clientState().manifestStatus(), ManifestStatus::kAccepted);
    const Entity mirror = pair.clientState().find(1);
    ASSERT_TRUE(pair.client.app->getWorld().getComponents().has(mirror, pair.client.shield.id));
    EXPECT_EQ(pair.client.read<std::uint32_t>(mirror, "hp"), 100U);
    EXPECT_EQ(pair.client.read<aylo::ecs::String>(mirror, "owner"), aylo::ecs::String{"Ana"});

    pair.server.write(shield, "hp", std::uint32_t{40});
    pair.server.write(shield, "owner", aylo::ecs::String{"Bo"});
    ASSERT_TRUE(
        runUntil(pair.server, pair.client, [&] { return pair.client.read<std::uint32_t>(mirror, "hp") == 40U; }));
    EXPECT_EQ(pair.client.read<aylo::ecs::String>(mirror, "owner"), aylo::ecs::String{"Bo"});
}

TEST(ScriptReplication, AgreesWhateverTheOrderOfTheFieldsInTheScript) {
    auto reversed = shieldFields();
    std::ranges::reverse(reversed);
    Pair pair{makeShield(shieldFields()), makeShield(reversed)};
    static_cast<void>(pair.spawnShield(7, "Ana"));

    ASSERT_TRUE(runUntil(pair.server, pair.client, [&] { return pair.clientState().find(1) != Entity{}; }));

    EXPECT_EQ(pair.clientState().manifestStatus(), ManifestStatus::kAccepted);
    EXPECT_EQ(pair.client.read<std::uint32_t>(pair.clientState().find(1), "hp"), 7U);
}

TEST(ScriptReplication, RefusesAClientWhoseLayoutDiffers) {
    auto other = shieldFields();
    other.push_back({.name = "armor", .type = FieldType::kF32});
    Pair pair{makeShield(shieldFields()), makeShield(other)};

    ASSERT_TRUE(runUntil(pair.server, pair.client,
                         [&] { return pair.clientState().manifestStatus() != ManifestStatus::kWaiting; }));

    EXPECT_EQ(pair.clientState().manifestStatus(), ManifestStatus::kMismatch);
    EXPECT_NE(pair.clientState().mismatch().find("game.Shield"), std::string_view::npos);
    EXPECT_EQ(pair.serverState().clientCount(), 0U);
}

TEST(ScriptReplication, RefusesAClientThatDidNotLoadTheScript) {
    Pair pair{makeShield(shieldFields()), makeShield(shieldFields())};
    // A third App whose scripts declare no `game.Shield` at all.
    App bare{RunMode::kClient};
    bare.addPlugin(aylo::ecs::TimePlugin::manual(kFrame, aylo::ecs::FixedTimestep{kTickRate, 5}));
    bare.addPlugin(aylo::replication::ReplicationPlugin{});
    bare.getWorld().getResources().getMut<aylo::replication::input::InputLayout>()->add(
        "fire", aylo::replication::input::InputKind::kButton);
    const auto address = network::io::Address::resolve("127.0.0.1", pair.serverState().netServer().localPort());
    bare.addPlugin(ClientPlugin{address.value(), makeNetworkSettings()});
    auto& state = *bare.getWorld().getResources().getMut<ClientState>();

    const auto deadline = network::Clock::now() + 2s;
    while (state.manifestStatus() == ManifestStatus::kWaiting && network::Clock::now() < deadline) {
        pair.server.app->runFrame();
        bare.runFrame();
        std::this_thread::sleep_for(1ms);
    }

    EXPECT_EQ(state.manifestStatus(), ManifestStatus::kMismatch);
    EXPECT_NE(state.mismatch().find("game.Shield"), std::string_view::npos);
}

TEST(ScriptReplication, SendsAComponentDeclaredInLuauToTheClient) {
    const ScriptsDirectory serverScripts{"aylo-script-replication-server", shieldScript(kShieldFields)};
    const ScriptsDirectory clientScripts{"aylo-script-replication-client", shieldScript(kShieldFields)};
    Pair pair{Game{RunMode::kServer, serverScripts.path()}, Game{RunMode::kClient, clientScripts.path()}};
    ASSERT_NE(pair.server.shield.fields.size(), 0U);
    const Entity shield = pair.spawnShield(100, "Ana");

    ASSERT_TRUE(runUntil(pair.server, pair.client, [&] { return pair.clientState().find(1) != Entity{}; }));

    EXPECT_EQ(pair.clientState().manifestStatus(), ManifestStatus::kAccepted);
    const Entity mirror = pair.clientState().find(1);
    EXPECT_EQ(pair.client.read<std::uint32_t>(mirror, "hp"), 100U);
    EXPECT_EQ(pair.client.read<aylo::ecs::String>(mirror, "owner"), aylo::ecs::String{"Ana"});

    pair.server.write(shield, "hp", std::uint32_t{40});
    ASSERT_TRUE(
        runUntil(pair.server, pair.client, [&] { return pair.client.read<std::uint32_t>(mirror, "hp") == 40U; }));
}

TEST(ScriptReplication, RefusesAClientWhoseLuauDeclaresADifferentLayout) {
    const ScriptsDirectory serverScripts{"aylo-script-replication-same", shieldScript(kShieldFields)};
    const ScriptsDirectory clientScripts{"aylo-script-replication-other",
                                         shieldScript(R"(hp = "u32", owner = "string", armor = "f32")")};
    Pair pair{Game{RunMode::kServer, serverScripts.path()}, Game{RunMode::kClient, clientScripts.path()}};

    ASSERT_TRUE(runUntil(pair.server, pair.client,
                         [&] { return pair.clientState().manifestStatus() != ManifestStatus::kWaiting; }));

    EXPECT_EQ(pair.clientState().manifestStatus(), ManifestStatus::kMismatch);
    EXPECT_NE(pair.clientState().mismatch().find("game.Shield"), std::string_view::npos);
    EXPECT_EQ(pair.serverState().clientCount(), 0U);
}
