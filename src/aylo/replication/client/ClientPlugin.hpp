/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ClientPlugin
*/

#pragma once

#include <string_view>

#include "aylo/ecs/app/Plugin.hpp"
#include "aylo/network/NetworkSettings.hpp"
#include "aylo/network/io/Address.hpp"
#include "aylo/replication/Export.hpp"

namespace aylo::ecs {
class App;
}  // namespace aylo::ecs

namespace aylo::replication::client {
/// @brief The client's replication: connects, checks the manifest, applies snapshots and sends the player's inputs.
///
/// @details An ordinary plugin: build() only registers (the ReplicationPlugin, the ClientState resource and the
/// systems, all on the client side), and the work is done by systems on the phases of the frame:
///
/// | Phase       | System          | What it does                                                              |
/// |-------------|-----------------|---------------------------------------------------------------------------|
/// | Startup     | `net.connect`   | opens the socket and starts connecting                                    |
/// | PreUpdate   | `net.receive`   | polls the transport; checks the manifest; applies the snapshots           |
/// | PostUpdate  | `net.sendInput` | numbers the input the game gave and sends it with the two before          |
///
/// The entities of a snapshot are created by commands, so they exist from the phase after PreUpdate. The game reads
/// the player's input and calls ClientState::setInput() in a system that runs
/// `.before(ClientPlugin::kSendInputSystem)`. Add the plugin after the ones that register replicated components.
/// @code
/// app.addPlugin(GamePlugin{});
/// app.addPlugin(aylo::replication::client::ClientPlugin{server, aylo::network::NetworkSettings{}});
/// app.addSystem(aylo::ecs::Phase::kPostUpdate, "game.input", &sendInput)
///     .before(std::string{aylo::replication::client::ClientPlugin::kSendInputSystem});
/// @endcode
class AYLO_REPLICATION_API ClientPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "aylo.replication.client";  ///< The plugin's name.
    static constexpr std::string_view kConnectSystem = "net.connect";     ///< Startup: connects.
    static constexpr std::string_view kReceiveSystem = "net.receive";     ///< PreUpdate: receives.
    static constexpr std::string_view kSendInputSystem =
        "net.sendInput";  ///< PostUpdate: the game's input runs before.

    /// @param server The address of the server.
    /// @param settings The transport settings.
    ClientPlugin(const network::io::Address& server, const network::NetworkSettings& settings);
    ~ClientPlugin() override;
    ClientPlugin(const ClientPlugin& other) = default;
    ClientPlugin& operator=(const ClientPlugin& other) = default;
    ClientPlugin(ClientPlugin&& other) noexcept = default;
    ClientPlugin& operator=(ClientPlugin&& other) noexcept = default;

    /// @return "aylo.replication.client".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Adds the ReplicationPlugin, inserts the ClientState and adds the systems.
    void build(ecs::App& app) override;

  private:
    network::io::Address _server;        ///< The address of the server.
    network::NetworkSettings _settings;  ///< The transport settings.
};
}  // namespace aylo::replication::client
