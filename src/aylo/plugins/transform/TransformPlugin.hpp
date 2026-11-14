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
class PluginRegistry;
}  // namespace aylo::ecs

namespace aylo::plugins::transform {

/// @brief Registers the transform's script forms (`engine.math.quat`, the rotations of Transform3D) in @p registry:
/// called by the library, so a side that does not build the plugin still gets them, and by build().
AYLO_PLUGIN_TRANSFORM_API void addTransformForms(ecs::PluginRegistry& registry);

/// @brief Positions entities of both families: registers Transform2D, GlobalTransform2D, Transform3D,
/// GlobalTransform3D and TransformInterpolation, and computes each world pose from its transform and the entity's
/// parents (ecs::Parent). Each family is propagated on its own: a parent of the other family is ignored, its child
/// treated as a root (docs/3d/design.md).
///
/// @details Three PostUpdate systems, each for both families:
/// - "aylo.Transform.Interpolate", on the client (and standalone): records the pose of every entity with a
///   TransformInterpolation after the fixed ticks and blends the last two by Time::alpha.
/// The two others run on every side (Side::kBoth: the server's gameplay and the client's rendering need them):
/// - "aylo.Transform.Insert": gives a world pose to every entity that has a transform but no world pose yet (spawned
///   this frame), combined with its parents; the commands apply at the end of PostUpdate, so Render sees it.
/// - "aylo.Transform.Propagate", after both: computes every world pose from the roots down (from the blended pose of an
///   interpolated entity), writing only those that differ, so `Changed<GlobalTransform2D>` only reports entities that
///   moved.
/// Systems that move entities write the transform before PostUpdate; systems that read positions (rendering) read the
/// world pose after it.
class AYLO_PLUGIN_TRANSFORM_API TransformPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "aylo.Transform";                           ///< The plugin's name.
    static constexpr std::string_view kInsertSystem = "aylo.Transform.Insert";            ///< Adds world poses.
    static constexpr std::string_view kPropagateSystem = "aylo.Transform.Propagate";      ///< Updates world poses.
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
