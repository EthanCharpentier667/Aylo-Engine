/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ReplicationPlugin
*/

#include "aylo/replication/ReplicationPlugin.hpp"

#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/event/Events.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/replication/client/ClientCalls.hpp"
#include "aylo/replication/component/NetworkId.hpp"
#include "aylo/replication/component/Owner.hpp"
#include "aylo/replication/component/PlayerInput.hpp"
#include "aylo/replication/event/ClientConnected.hpp"
#include "aylo/replication/event/ClientDisconnected.hpp"
#include "aylo/replication/input/InputCalls.hpp"
#include "aylo/replication/input/InputLayout.hpp"
#include "aylo/replication/packet/PacketCalls.hpp"
#include "aylo/replication/packet/PacketSchema.hpp"
#include "aylo/replication/standalone/LocalSession.hpp"

namespace aylo::replication {
ReplicationPlugin::~ReplicationPlugin() = default;  // defined here: the class's vtable lives in the library

std::string_view ReplicationPlugin::getName() const noexcept { return kName; }

void ReplicationPlugin::build(ecs::App& app) {
    ecs::Components& components = app.getWorld().getComponents();
    static_cast<void>(components.registerType<component::NetworkId>(
        {.presence = ecs::Presence::kBoth, .replication = ecs::ReplicationMode::kNone}));
    static_cast<void>(components.registerType<component::Owner>(
        {.presence = ecs::Presence::kBoth, .replication = ecs::ReplicationMode::kEveryChange}));
    static_cast<void>(components.registerType<ecs::Parent>(ecs::kParentOptions));
    static_cast<void>(components.registerType<component::PlayerInput>(
        {.presence = ecs::Presence::kBoth, .replication = ecs::ReplicationMode::kNone}));

    ecs::Events& events = app.getWorld().getEvents();
    static_cast<void>(events.registerType<event::ClientConnected>({.presence = ecs::Presence::kServerOnly}));
    static_cast<void>(events.registerType<event::ClientDisconnected>({.presence = ecs::Presence::kServerOnly}));

    static_cast<void>(events.registerType<packet::PacketReceived>({.presence = ecs::Presence::kBoth}));

    app.insertResourceIfAbsent<input::InputLayout>();
    app.insertResourceIfAbsent<packet::PacketRegistry>();
    packet::addPacketCalls(app);
    input::addInputCalls(app);
    // A standalone session has no ClientState: the StandalonePlugin answers localClient.
    const ecs::RunMode mode = app.getSchedules().getRunMode();
    if (!app.getWorld().getResources().has<standalone::LocalSession>() && ecs::runsIn(ecs::Side::kClient, mode)) {
        client::addClientCalls(app);
    }
}
}  // namespace aylo::replication
