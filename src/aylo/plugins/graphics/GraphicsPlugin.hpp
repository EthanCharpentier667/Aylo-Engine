/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GraphicsPlugin
*/

#pragma once

#include <memory>
#include <span>
#include <string_view>

#include "aylo/ecs/app/Plugin.hpp"
#include "core/config/Settings.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/Export.hpp"

namespace aylo::ecs {
class App;
}  // namespace aylo::ecs

namespace aylo::plugins::graphics {
/// @brief The window and the renderer for an App: starts the platform and the renderer the configuration names,
/// turns the platform's events into ECS events, and frames the drawing.
///
/// @details The registry comes filled by the executable (the only place that knows GLFW and Vulkan), so this plugin
/// depends on no backend. It adds client systems only, so a dedicated server has no window:
/// - Startup "aylo.graphics.create": creates the Backend (BackendRegistry::createBackend()) into the main-thread
///   resource "aylo.Backend".
/// - PreUpdate "aylo.graphics.events": polls the platform (or waits for an event while the window is minimized),
///   sends every event as a PlatformEvent, in order, and as its typed ECS event (the input ones of InputEvents.hpp,
///   and Closed, Resized, TextEntered), resizes the renderer, and sends AppExit once the window is closed. It runs
///   before the InputPlugin's system. A tool that shows the game inside its own window sets the PlatformEventFilter
///   resource, which sees every event first and may change it or hide it from the game.
/// - Render "aylo.graphics.textures": destroys the unloaded textures and loads the queued ones of the TextureAssets
///   resource (updateTextures()), so a texture asked for during this frame's Update is drawn in this frame.
/// - Render "aylo.graphics.beginFrame" and "aylo.graphics.endFrame": clear with the ClearColor resource, and show the
///   frame (a tool that draws over the game sets the ToolFrame resource, which endFrame calls just before it ends the
///   frame). In between, "aylo.graphics.camera" shows the world from the entity with a Camera (the highest priority)
///   and a GlobalTransform, through IRenderer::setView() (none: the renderer's default view, window pixels from the
///   top-left corner). Drawing systems run between the two (`.after("aylo.graphics.beginFrame")`,
///   `.before("aylo.graphics.endFrame")`). Both are skipped while the window is minimized. The camera system also
///   records what the frame shows in the VisibleArea resource, for systems that skip what is off screen.
/// - Render "aylo.graphics.sprites", after the camera: draws what the plugins pushed to the SpriteQueue resource
///   before it, sorted by layer (higher on top), then by texture within a layer (fewer batches), and empties the queue.
/// It also inserts the TextureAssets (the textures by AssetId), SpriteQueue and VisibleArea resources. It requires the
/// InputPlugin, as its events system runs before the InputPlugin's, and the TransformPlugin, for the cameras.
/// It adds no plugin: the game lists what it draws (SpriteDrawPlugin, TilemapPlugin, TextPlugin, ParticlePlugin,
/// ShaderPlugin), and they know this plugin to run between its systems.
/// @code
/// auto registry = std::make_unique<aylo::plugins::graphics::BackendRegistry>();
/// aylo::platform::glfw::registerPlatform(registry->platforms);
/// aylo::render::vulkan::registerRenderer(registry->renderers);
/// app.addPlugin(aylo::plugins::transform::TransformPlugin{});
/// app.addPlugin(aylo::plugins::input::InputPlugin{});
/// app.addPlugin(aylo::plugins::graphics::GraphicsPlugin{std::move(registry), settings});
/// @endcode
class AYLO_PLUGIN_GRAPHICS_API GraphicsPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "aylo.graphics";                     ///< The plugin's name.
    static constexpr std::string_view kCreateSystem = "aylo.graphics.create";      ///< Startup: creates the Backend.
    static constexpr std::string_view kEventsSystem = "aylo.graphics.events";      ///< PreUpdate: sends the events.
    static constexpr std::string_view kTexturesSystem = "aylo.graphics.textures";  ///< Render: loads the textures.
    static constexpr std::string_view kBeginFrameSystem = "aylo.graphics.beginFrame";  ///< Render: starts the frame.
    static constexpr std::string_view kCameraSystem = "aylo.graphics.camera";          ///< Render: applies the camera.
    static constexpr std::string_view kSpritesSystem = "aylo.graphics.sprites";        ///< Render: draws the sprites.
    static constexpr std::string_view kEndFrameSystem = "aylo.graphics.endFrame";      ///< Render: shows the frame.

    /// @param registry The platforms and renderers, registered by the executable.
    /// @param settings The configuration: `platform`, `renderer`, `window` and a section per backend.
    GraphicsPlugin(std::unique_ptr<plugins::graphics::BackendRegistry> registry, core::config::Settings settings);
    ~GraphicsPlugin() override = default;
    GraphicsPlugin(const GraphicsPlugin& other) = delete;
    GraphicsPlugin& operator=(const GraphicsPlugin& other) = delete;
    GraphicsPlugin(GraphicsPlugin&& other) noexcept = default;
    GraphicsPlugin& operator=(GraphicsPlugin&& other) noexcept = default;

    /// @return "aylo.graphics".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @return The InputPlugin, which reads the events this plugin sends, and the TransformPlugin.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;

    /// @brief Registers the window and input events, inserts the resources and adds the systems. Opens nothing: the
    /// window is created by the Startup system.
    void build(ecs::App& app) override;

  private:
    std::unique_ptr<plugins::graphics::BackendRegistry> _registry;  ///< Handed to the Startup system by build().
    core::config::Settings _settings;                               ///< Handed to the Startup system by build().
};
}  // namespace aylo::plugins::graphics
