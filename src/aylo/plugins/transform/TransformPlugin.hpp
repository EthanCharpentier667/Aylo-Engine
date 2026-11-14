/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TransformPlugin
*/

#pragma once

#include <string_view>

#include "aylo/ecs/app/Plugin.hpp"
#include "plugins/transform/Export.hpp"

namespace aylo::ecs {
class App;
}  // namespace aylo::ecs

namespace aylo::plugins::transform {

/// @brief Positions entities: registers Transform2D, GlobalTransform2D and TransformInterpolation, and computes
/// GlobalTransform2D from Transform2D and the entity's parents (ecs::Parent).
///
/// @details Three PostUpdate systems:
/// - "aylo.Transform.Interpolate", on the client (and standalone): records the pose of every entity with a
///   TransformInterpolation after the fixed ticks and blends the last two by Time::alpha.
/// The two others run on every side (Side::kBoth: the server's gameplay and the client's rendering need them):
/// - "aylo.Transform.Insert": gives a GlobalTransform2D to every entity that has a Transform2D but no GlobalTransform2D yet
///   (spawned this frame), combined with its parents; the commands apply at the end of PostUpdate, so Render sees it.
/// - "aylo.Transform.Propagate", after both: computes every GlobalTransform2D from the roots down (from the blended pose
///   of an interpolated entity), writing only those that differ, so `Changed<GlobalTransform2D>` only reports
///   entities that moved. A parent without a Transform2D is ignored: its child is treated as a root.
/// Systems that move entities write Transform2D before PostUpdate; systems that read positions (rendering) read
/// GlobalTransform2D after it.
class AYLO_PLUGIN_TRANSFORM_API TransformPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "aylo.Transform";                             ///< The plugin's name.
    static constexpr std::string_view kInsertSystem = "aylo.Transform.Insert";            ///< Adds GlobalTransform2D.
    static constexpr std::string_view kPropagateSystem = "aylo.Transform.Propagate";      ///< Updates GlobalTransform2D.
    static constexpr std::string_view kInterpolateSystem = "aylo.Transform.Interpolate";  ///< Blends the fixed ticks.

    TransformPlugin() = default;
    ~TransformPlugin() override = default;
    TransformPlugin(const TransformPlugin& other) = default;
    TransformPlugin& operator=(const TransformPlugin& other) = default;
    TransformPlugin(TransformPlugin&& other) noexcept = default;
    TransformPlugin& operator=(TransformPlugin&& other) noexcept = default;

    /// @return "aylo.Transform".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Registers the components and adds the PostUpdate systems.
    void build(ecs::App& app) override;
};

}  // namespace aylo::plugins::transform
