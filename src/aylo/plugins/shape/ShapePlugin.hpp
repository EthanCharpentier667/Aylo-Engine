/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShapePlugin
*/

#pragma once

#include <span>
#include <string_view>

#include "aylo/ecs/app/Plugin.hpp"
#include "plugins/shape/Export.hpp"

namespace aylo::ecs {
class App;
}  // namespace aylo::ecs

namespace aylo::plugins::shape {

/// @brief Draws the ShapeRenderer entities on the client: walls, panels, bars and markers made of plain shapes.
///
/// @details Registers the ShapeRenderer component, on every side so that a server can hold and replicate it, and adds
/// the client Render system "aylo.Shape.Draw", after the GraphicsPlugin's sprites and before the end of the frame: it
/// draws every entity with a GlobalTransform2D and a ShapeRenderer, sorted by layer, through the renderer. The renderer
/// draws shapes as overlays, so they come over the sprites. It requires the GraphicsPlugin and the TransformPlugin.
/// @code
/// app.addPlugin(aylo::plugins::transform::TransformPlugin{})
///     .addPlugin(aylo::plugins::input::InputPlugin{})
///     .addPlugin(aylo::plugins::graphics::GraphicsPlugin{std::move(registry), settings})
///     .addPlugin(aylo::plugins::shape::ShapePlugin{});
/// @endcode
class AYLO_PLUGIN_SHAPE_API ShapePlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "aylo.Shape";             ///< The plugin's name.
    static constexpr std::string_view kDrawSystem = "aylo.Shape.Draw";  ///< Draws the shapes.

    ShapePlugin() = default;
    ~ShapePlugin() override = default;
    ShapePlugin(const ShapePlugin& other) = default;
    ShapePlugin& operator=(const ShapePlugin& other) = default;
    ShapePlugin(ShapePlugin&& other) noexcept = default;
    ShapePlugin& operator=(ShapePlugin&& other) noexcept = default;

    /// @return "aylo.Shape".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @return The GraphicsPlugin, which it draws through, and the TransformPlugin.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;

    /// @brief Registers the ShapeRenderer component and adds the client Render system.
    void build(ecs::App& app) override;
};

}  // namespace aylo::plugins::shape
