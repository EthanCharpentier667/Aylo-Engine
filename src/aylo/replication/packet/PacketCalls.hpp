/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PacketCalls
*/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/replication/Export.hpp"

namespace aylo::replication::packet {
inline constexpr std::string_view kDeclareCall = "aylo.net.declarePacket";        ///< PacketRegistry::declare().
inline constexpr std::string_view kListCall = "aylo.net.listPackets";             ///< Describes the declared packets.
inline constexpr std::string_view kSendToServerCall = "aylo.net.sendToServer";    ///< Client: sends a packet.
inline constexpr std::string_view kSendToClientsCall = "aylo.net.sendToClients";  ///< Server: sends a packet.

/// @brief The arguments of `aylo.net.declarePacket`.
struct DeclareArguments {
    std::string name;       ///< `set_directive`, `game.Buy`...
    std::string direction;  ///< "toServer" or "toClients".
    std::string channel;    ///< "reliable" or "unreliable".
    std::string fields;     ///< One `name:type` or `name:type:maxLength` per line, in wire order.
};
AYLO_ECS_CALL_DATA(DeclareArguments, "aylo.net.DeclareArguments", name, direction, channel, fields);

/// @brief The arguments of `aylo.net.listPackets`.
struct ListArguments {
    std::string prefix;  ///< Only the packets whose name starts with it; empty for every packet.
};
AYLO_ECS_CALL_DATA(ListArguments, "aylo.net.ListArguments", prefix);

/// @brief The result of `aylo.net.listPackets`: what `engine.net` needs to send and read packets declared natively.
struct ListResult {
    std::string packets;  ///< One packet per line: its name, then ` field:type` for each field, in wire order.
};
AYLO_ECS_CALL_DATA(ListResult, "aylo.net.ListResult", packets);

/// @brief The arguments of `aylo.net.sendToServer`.
struct SendToServerArguments {
    std::string name;    ///< A declared `toServer` packet.
    std::string values;  ///< The fields as parseValues() reads them.
};
AYLO_ECS_CALL_DATA(SendToServerArguments, "aylo.net.SendToServerArguments", name, values);

/// @brief The arguments of `aylo.net.sendToClients`.
struct SendToClientsArguments {
    std::string name;         ///< A declared `toClients` packet.
    std::uint32_t client{0};  ///< The client to send it to; 0 for every connected client.
    std::string values;       ///< The fields as parseValues() reads them.
};
AYLO_ECS_CALL_DATA(SendToClientsArguments, "aylo.net.SendToClientsArguments", name, client, values);

/// @brief The result of both send calls.
struct SendResult {
    bool sent{false};  ///< False when nobody could get it: not connected, or no such client.
};
AYLO_ECS_CALL_DATA(SendResult, "aylo.net.SendResult", sent);

/// @brief A packet that arrived and passed every check of its schema. The server's scripts get the clients' packets
/// and the client's scripts the server's, in PreUpdate.
struct PacketReceived {
    std::string name;         ///< The packet's name.
    std::uint32_t sender{0};  ///< On the server, the client that sent it; 0 on a client.
    std::string values;       ///< The fields as formatValues() writes them; `engine.net.received` makes a table.
};
AYLO_ECS_EVENT(PacketReceived, "net.PacketReceived", name, sender, values);

/// @brief Registers `aylo.net.declarePacket` everywhere, `aylo.net.sendToServer` on the client side and
/// `aylo.net.sendToClients` on the server side, and the Luau forms (`engine.net`). Called by the ReplicationPlugin.
AYLO_REPLICATION_API void addPacketCalls(ecs::App& app);
}  // namespace aylo::replication::packet
