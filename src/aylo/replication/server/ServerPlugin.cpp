/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ServerPlugin
*/

#include "aylo/replication/server/ServerPlugin.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryMatch.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/DynamicQueries.hpp"
#include "aylo/ecs/system/EventWriter.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/time/Time.hpp"
#include "aylo/network/NetworkSettings.hpp"
#include "aylo/network/admin/AdminSettings.hpp"
#include "aylo/replication/ReplicationPlugin.hpp"
#include "aylo/replication/ReplicationSettings.hpp"
#include "aylo/replication/component/NetworkId.hpp"
#include "aylo/replication/component/Owner.hpp"
#include "aylo/replication/component/PlayerInput.hpp"
#include "aylo/replication/event/ClientConnected.hpp"
#include "aylo/replication/event/ClientDisconnected.hpp"
#include "aylo/replication/input/InputLayout.hpp"
#include "aylo/replication/packet/PacketCalls.hpp"
#include "aylo/replication/packet/PacketCodec.hpp"
#include "aylo/replication/packet/PacketSchema.hpp"
#include "aylo/replication/query/ReplicatedQueries.hpp"
#include "aylo/replication/server/ServerState.hpp"

namespace aylo::replication::server {
namespace {
/// @brief Startup: opens the server.
void start(ecs::ResMut<ServerState> state, ecs::ResMut<input::InputLayout> inputs,
           ecs::ResMut<packet::PacketRegistry> packets) {
    packets->freeze();
    state->start(*inputs);
}

/// @brief PreUpdate: receives, and tells the game who joined and left.
void receive(ecs::ResMut<ServerState> state, ecs::Res<input::InputLayout> inputs, ecs::Commands& commands,
             ecs::EventWriter<event::ClientConnected> connected, ecs::EventWriter<event::ClientDisconnected> left,
             ecs::Res<packet::PacketRegistry> packets, ecs::EventWriter<packet::PacketReceived> received) {
    state->bindCommands(&commands);  // the admin channel records its world changes there, applied after the phase
    const std::vector<ServerNotification> notifications = state->receive(inputs.get(), &*packets);
    state->bindCommands(nullptr);
    for (const ServerNotification& notification : notifications) {
        if (const auto* joined = std::get_if<event::ClientConnected>(&notification)) {
            connected.send(*joined);
        } else {
            left.send(std::get<event::ClientDisconnected>(notification));
        }
    }
    // Only what passes the schema reaches the scripts: the rest is dropped, and they validate what it means.
    for (const ServerState::RawPacket& raw : state->takePackets()) {
        auto decoded = packet::decodePacket(*packets, packet::PacketDirection::kToServer, raw.payload);
        const packet::PacketSchema* schema = decoded.has_value() ? packets->find(decoded->name) : nullptr;
        // A reliable packet sent unreliably could be duplicated or reordered: it is refused too.
        if (decoded.has_value() && schema != nullptr && schema->channel == raw.channel) {
            received.send(packet::PacketReceived{
                .name = decoded->name,
                .sender = raw.client,
                .values = packet::formatValues(*schema, decoded->values),
            });
        } else {
            state->noteDroppedPacket();
        }
    }
}

/// @brief FixedUpdate: writes the next input of each client in the entity it owns.
void applyInputs(ecs::ResMut<ServerState> state, ecs::Query<component::PlayerInput, const component::Owner> players) {
    for (auto [entity, input, owner] : players) {
        state->applyInput(owner.client, *input);
    }
}

/// @brief PostUpdate: gives a NetworkId to the entities that have a replicated component and none.
void assignIds(ecs::ResMut<ServerState> state, ecs::Commands& commands, ecs::DynamicQueries untracked) {
    for (std::size_t query = 0; query < untracked.getCount(); ++query) {
        for (const ecs::QueryMatch& match : untracked.iter(query)) {
            if (const auto networkId = state->track(match.getEntity())) {
                commands.add(match.getEntity(), component::NetworkId{.value = *networkId});
            }
        }
    }
}

/// @brief PostUpdate: sends a snapshot to every client, when one is due.
void snapshot(ecs::ResMut<ServerState> state, ecs::Res<ecs::Time> time, ecs::DynamicQueries replicated) {
    const std::uint32_t tick = time->tick;
    if (state->isSnapshotDue(tick)) {
        state->sendSnapshots(tick, replicated);
    }
}
}  // namespace

ServerPlugin::ServerPlugin(const network::NetworkSettings& network, const ReplicationSettings& settings)
    : _network{network}, _settings{settings} {}

ServerPlugin::~ServerPlugin() = default;

void ServerPlugin::setAdmin(network::admin::AdminSettings settings) { _admin = std::move(settings); }

std::string_view ServerPlugin::getName() const noexcept { return kName; }

void ServerPlugin::build(ecs::App& app) {
    app.addPlugin(ReplicationPlugin{});
    ServerState state{_network, _settings, app.getWorld()};
    if (_admin) {
        state.setAdmin(*_admin);
    }
    state.setSchedules(&app.getSchedules());
    app.insertResource(std::move(state));

    app.addSystem(ecs::Phase::kStartup, std::string{kStartSystem}, &start);
    app.addSystem(ecs::Phase::kPreUpdate, std::string{kReceiveSystem}, &receive);
    app.addSystem(ecs::Phase::kFixedUpdate, std::string{kApplyInputsSystem}, &applyInputs);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kAssignIdsSystem}, &assignIds,
                  query::makeUntrackedQueries(app.getWorld()));
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kSnapshotSystem}, &snapshot,
                  query::makeReplicatedReads(app.getWorld()))
        .after(std::string{kAssignIdsSystem});
}
}  // namespace aylo::replication::server
