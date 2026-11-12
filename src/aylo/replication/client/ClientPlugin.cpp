/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ClientPlugin
*/

#include "aylo/replication/client/ClientPlugin.hpp"

#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/EventWriter.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/network/NetworkSettings.hpp"
#include "aylo/network/Types.hpp"
#include "aylo/network/io/Address.hpp"
#include "aylo/replication/ReplicationPlugin.hpp"
#include "aylo/replication/client/ClientState.hpp"
#include "aylo/replication/input/InputLayout.hpp"
#include "aylo/replication/packet/PacketCalls.hpp"
#include "aylo/replication/packet/PacketCodec.hpp"
#include "aylo/replication/packet/PacketSchema.hpp"

namespace aylo::replication::client {
namespace {
/// @brief Startup: opens the socket and starts connecting.
void connect(ecs::ResMut<ClientState> state, ecs::ResMut<packet::PacketRegistry> packets) {
    packets->freeze();
    state->connect();
}

/// @brief PreUpdate: receives, checks the manifest and records the snapshots in the commands.
void receive(ecs::ResMut<ClientState> state, ecs::ResMut<input::InputLayout> inputs, ecs::Commands& commands,
             ecs::Res<packet::PacketRegistry> packets, ecs::EventWriter<packet::PacketReceived> received) {
    state->receive(*inputs, commands);
    // Only what passes the schema reaches the scripts: the rest is dropped.
    for (const network::Bytes& message : state->takePackets()) {
        if (auto decoded = packet::decodePacket(*packets, packet::PacketDirection::kToClients, message)) {
            const packet::PacketSchema* schema = packets->find(decoded->name);
            received.send(packet::PacketReceived{
                .name = decoded->name,
                .sender = 0,
                .values = packet::formatValues(*schema, decoded->values),
            });
        } else {
            state->noteDroppedPacket();
        }
    }
}

/// @brief PostUpdate: sends the input of the frame.
void sendInput(ecs::ResMut<ClientState> state, ecs::Res<input::InputLayout> inputs) { state->sendInput(*inputs); }
}  // namespace

ClientPlugin::ClientPlugin(const network::io::Address& server, const network::NetworkSettings& settings)
    : _server{server}, _settings{settings} {}

ClientPlugin::~ClientPlugin() = default;

std::string_view ClientPlugin::getName() const noexcept { return kName; }

void ClientPlugin::build(ecs::App& app) {
    app.addPlugin(ReplicationPlugin{});
    app.insertResource(ClientState{_server, _settings, app.getWorld()});

    app.addSystem(ecs::Phase::kStartup, std::string{kConnectSystem}, &connect).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPreUpdate, std::string{kReceiveSystem}, &receive).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kSendInputSystem}, &sendInput).setSide(ecs::Side::kClient);
}
}  // namespace aylo::replication::client
