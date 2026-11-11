/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PhysicsDebugPlugin
*/

#pragma once

#include <optional>
#include <span>
#include <string_view>

#include "aylo/ecs/app/Plugin.hpp"
#include "plugins/physics/debug/Export.hpp"
#include "plugins/physics/debug/PhysicsDebugSettings.hpp"

namespace aylo::ecs {
class App;
}  // namespace aylo::ecs

namespace aylo::plugins::physics::debug {

/// @brief Draws the physics of chosen entities on the client: collider outlines and velocity arrows.
///
/// @details The PhysicsDebug component chooses the entities (as gizmos in Unity); PhysicsDebugSettings holds the global
/// switches: enabled (flipped by a key, F3 by default) and drawAll (every collider, PhysicsDebug or not). The plugin
/// does all the drawing, from the components (Collider, GlobalTransform, Velocity), so it shows exactly where sprites
/// are drawn and needs no physics body. Its systems run on the client side (so also in standalone):
/// - "aylo.PhysicsDebug.Toggle", in Update: flips PhysicsDebugSettings::enabled on the toggle key;
/// - "aylo.PhysicsDebug.Draw", in Render after the sprites and the ShapeRenderers: draws the outlines and arrows as
/// shapes (shapes are
///   overlays, drawn on top of the sprites).
/// It draws through the GraphicsPlugin's renderer, so add it to clients with a window only; the headless server never
/// does. Registers PhysicsDebug, and inserts PhysicsDebugSettings if the App has none.
/// @code
/// app.addPlugin(aylo::plugins::transform::TransformPlugin{})
///     .addPlugin(aylo::plugins::input::InputPlugin{})
///     .addPlugin(aylo::plugins::graphics::GraphicsPlugin{std::move(registry), settings})
///     .addPlugin(aylo::plugins::physics::debug::PhysicsDebugPlugin{});
/// @endcode
class AYLO_PLUGIN_PHYSICS_DEBUG_API PhysicsDebugPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "aylo.PhysicsDebug";                 ///< The plugin's name.
    static constexpr std::string_view kToggleSystem = "aylo.PhysicsDebug.Toggle";  ///< Flips enabled on the key.
    static constexpr std::string_view kDrawSystem = "aylo.PhysicsDebug.Draw";      ///< Draws the outlines and arrows.

    /// @param settings What it inserts, replacing the default ones.
    explicit PhysicsDebugPlugin(PhysicsDebugSettings settings);
    PhysicsDebugPlugin() = default;
    ~PhysicsDebugPlugin() override = default;
    PhysicsDebugPlugin(const PhysicsDebugPlugin& other) = default;
    PhysicsDebugPlugin& operator=(const PhysicsDebugPlugin& other) = default;
    PhysicsDebugPlugin(PhysicsDebugPlugin&& other) noexcept = default;
    PhysicsDebugPlugin& operator=(PhysicsDebugPlugin&& other) noexcept = default;

    /// @return "aylo.PhysicsDebug".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @return The GraphicsPlugin, which it draws through, the InputPlugin, for its key, the ShapePlugin, which it
    /// draws after, and the TransformPlugin. Not the PhysicsPlugin: it only reads Collider, which it registers itself,
    /// so it also shows what a client receives.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;

    /// @brief Registers the components and events it reads, inserts the settings, and adds the client systems.
    void build(ecs::App& app) override;

  private:
    std::optional<PhysicsDebugSettings> _settings;  ///< What it inserts; nothing: the App's, or the default ones.
};

}  // namespace aylo::plugins::physics::debug
