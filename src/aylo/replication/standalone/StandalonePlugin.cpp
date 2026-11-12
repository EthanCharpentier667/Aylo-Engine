/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StandalonePlugin
*/

#include "aylo/replication/standalone/StandalonePlugin.hpp"

#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/EventWriter.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/replication/ReplicationPlugin.hpp"
#include "aylo/replication/client/ClientCalls.hpp"
#include "aylo/replication/component/Owner.hpp"
#include "aylo/replication/component/PlayerInput.hpp"
#include "aylo/replication/event/ClientConnected.hpp"
#include "aylo/replication/packet/PacketCalls.hpp"
#include "aylo/replication/packet/PacketCodec.hpp"
#include "aylo/replication/packet/PacketSchema.hpp"
#include "aylo/replication/standalone/LocalSession.hpp"

namespace aylo::replication::standalone {
namespace {
/// @brief `aylo.replication.localClient`: the local client is always connected.
client::LocalClient localClient() { return client::LocalClient{.connected = true, .client = LocalSession::kClient}; }

/// @brief PreUpdate: announces the local client once and delivers the packets sent since the last frame.
void receive(ecs::ResMut<LocalSession> session, ecs::Res<packet::PacketRegistry> packets,
             ecs::EventWriter<event::ClientConnected> connected, ecs::EventWriter<packet::PacketReceived> received) {
    if (session->takeConnect()) {
        connected.send(event::ClientConnected{.client = LocalSession::kClient});
    }
    for (const QueuedPacket& queued : session->takePackets()) {
        const auto decoded = packet::decodePacket(*packets, queued.direction, queued.message);
        if (!decoded) {
            continue;  // cannot happen for a packet the send call encoded; dropped like a bad network one
        }
        const packet::PacketSchema* schema = packets->find(decoded->name);
        if (schema == nullptr) {
            continue;
        }
        received.send(packet::PacketReceived{
            .name = decoded->name,
            .sender = queued.direction == packet::PacketDirection::kToServer ? LocalSession::kClient : 0,
            .values = packet::formatValues(*schema, decoded->values),
        });
    }
}

/// @brief FixedUpdate: writes the local input in the entity of client 1, as the server does with a client's.
void applyInputs(ecs::Res<LocalSession> session, ecs::Query<component::PlayerInput, const component::Owner> players) {
    for (auto [entity, input, owner] : players) {
        input->previousButtons = input->buttons;
        if (owner.client == LocalSession::kClient) {
            input->buttons = session->input().buttons;
            input->axes = session->input().axes;
            input->sequence = session->input().sequence;
        } else {
            input->buttons = 0;
            input->axes = {};
            input->sequence = 0;
        }
    }
}
}  // namespace

StandalonePlugin::~StandalonePlugin() = default;

std::string_view StandalonePlugin::getName() const noexcept { return kName; }

void StandalonePlugin::build(ecs::App& app) {
    // First: the ReplicationPlugin picks the local calls when it finds the session.
    app.insertResource(LocalSession{});
    app.addPlugin(ReplicationPlugin{});
    app.addCall(std::string{client::kLocalClientCall}, &localClient);
    app.addSystem(ecs::Phase::kPreUpdate, std::string{kReceiveSystem}, &receive);
    app.addSystem(ecs::Phase::kFixedUpdate, std::string{kApplyInputsSystem}, &applyInputs);
}
}  // namespace aylo::replication::standalone
