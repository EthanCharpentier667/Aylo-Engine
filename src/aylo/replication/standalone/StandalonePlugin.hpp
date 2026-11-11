/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StandalonePlugin
*/

#pragma once

#include <string_view>

#include "aylo/ecs/app/Plugin.hpp"
#include "aylo/replication/Export.hpp"

namespace aylo::ecs {
class App;
}  // namespace aylo::ecs

namespace aylo::replication::standalone {
/// @brief The replication of a standalone world: a server with one local client, client 1, and no socket, so that a
/// networked game written once runs unchanged.
///
/// @details Adds the ReplicationPlugin (so every component, event, call and form exists), the LocalSession resource
/// and these systems:
///
/// | Phase       | System             | What it does                                                           |
/// |-------------|--------------------|------------------------------------------------------------------------|
/// | PreUpdate   | `net.receive`      | raises `net.ClientConnected` for client 1 once; delivers the packets   |
/// | FixedUpdate | `net.applyInputs` | writes the local input in the PlayerInput of the entity client 1 owns  |
///
/// `aylo.replication.localClient` answers `{connected = true, client = 1}`, and `aylo.net.sendToServer` and
/// `aylo.net.sendToClients` loop the packet back into the world, one frame later, with the same checks as the
/// network. A packet to the server has `sender` 1, one to the clients has `sender` 0: scripts tell the side they are
/// for from it (`engine.net.received`). The player's input is given with LocalSession::setInput(), in a system that
/// runs before the FixedUpdate.
class AYLO_REPLICATION_API StandalonePlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "aylo.replication.standalone";   ///< The plugin's name.
    static constexpr std::string_view kReceiveSystem = "net.receive";          ///< PreUpdate.
    static constexpr std::string_view kApplyInputsSystem = "net.applyInputs";  ///< FixedUpdate.

    StandalonePlugin() = default;
    ~StandalonePlugin() override;
    StandalonePlugin(const StandalonePlugin& other) = default;
    StandalonePlugin& operator=(const StandalonePlugin& other) = default;
    StandalonePlugin(StandalonePlugin&& other) noexcept = default;
    StandalonePlugin& operator=(StandalonePlugin&& other) noexcept = default;

    /// @return "aylo.replication.standalone".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Inserts the LocalSession, adds the ReplicationPlugin, adds the call and the systems.
    void build(ecs::App& app) override;
};
}  // namespace aylo::replication::standalone
