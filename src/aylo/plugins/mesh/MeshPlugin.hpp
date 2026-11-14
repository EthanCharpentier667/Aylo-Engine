/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshPlugin
*/

#pragma once

#include <span>
#include <string_view>

#include "aylo/ecs/app/Plugin.hpp"
#include "plugins/mesh/Export.hpp"

namespace aylo::ecs {
class App;
}  // namespace aylo::ecs

namespace aylo::plugins::mesh {

/// @brief 3D models from glTF files (docs/3d/design.md, D3-03): inserts the MeshAssets resource, registers
/// MeshRenderer and `aylo.mesh.load` (`engine.assets.mesh`), and draws the models, unlit.
///
/// @details Two systems, on the client:
/// - Render "aylo.Mesh.Read", before the GraphicsPlugin loads the textures: reads each queued glTF file
///   (detail::loadGltf()) and asks TextureAssets for the textures of its parts (appended to, ecs::ResAppend).
/// - Render "aylo.Mesh.Draw", after the GraphicsPlugin's 3D lines and before its 2D camera: creates each mesh whose
///   textures are loaded (or failed: the part is then drawn without one) with IRenderer::createStaticMesh(), then
///   draws each MeshRenderer whose mesh is loaded at its GlobalTransform3D.
/// A file that cannot be read, or a renderer that does not draw 3D, fails the mesh with a message on the error output.
/// It requires the GraphicsPlugin and the TransformPlugin.
class AYLO_PLUGIN_MESH_API MeshPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "aylo.Mesh";             ///< The plugin's name.
    static constexpr std::string_view kReadSystem = "aylo.Mesh.Read";  ///< Reads the glTF files.
    static constexpr std::string_view kDrawSystem = "aylo.Mesh.Draw";  ///< Creates the meshes, draws the MeshRenderers.

    MeshPlugin() = default;
    ~MeshPlugin() override = default;
    MeshPlugin(const MeshPlugin& other) = default;
    MeshPlugin& operator=(const MeshPlugin& other) = default;
    MeshPlugin(MeshPlugin&& other) noexcept = default;
    MeshPlugin& operator=(MeshPlugin&& other) noexcept = default;

    /// @return "aylo.Mesh".
    [[nodiscard]] std::string_view getName() const noexcept override;
    /// @return The plugins this one needs.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;
    /// @brief Inserts the resource, registers the component and the call, and adds the systems.
    void build(ecs::App& app) override;
};

}  // namespace aylo::plugins::mesh
