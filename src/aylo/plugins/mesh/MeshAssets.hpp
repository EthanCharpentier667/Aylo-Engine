/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshAssets
*/

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "core/asset/AssetId.hpp"
#include "core/vfs/FileSource.hpp"
#include "plugins/mesh/Export.hpp"

namespace aylo::plugins::mesh {

namespace detail {
struct GltfModel;
}  // namespace detail

/// @brief Where a mesh is in its loading.
enum class MeshState : std::uint8_t {
    kQueued,           ///< Asked for, its file not read yet.
    kWaitingTextures,  ///< Read; waiting for its textures, then for the renderer to create it.
    kLoaded,           ///< Created by the renderer: drawn by the MeshRenderers that name it.
    kFailed,           ///< Could not be loaded; the entry says why.
};

/// @brief The 3D models, as the ECS resource "aylo.MeshAssets" the MeshPlugin inserts: a script or a system asks for a
/// glTF file by path and gets its AssetId at once; the plugin reads it, loads its textures (through TextureAssets) and
/// has the renderer create it, over the next frames.
/// @details load() only records the request, so a server without a renderer gets the same ids. A mesh is loaded once
/// per path.
class AYLO_PLUGIN_MESH_API MeshAssets {
  public:
    /// @brief One model.
    struct Entry {
        std::string path;                          ///< Its file, from the assets folder.
        MeshState state{MeshState::kQueued};       ///< Where it is in its loading.
        std::shared_ptr<detail::GltfModel> model;  ///< The file read, until the renderer created the mesh.
        std::vector<std::optional<core::asset::AssetId>> partTextures;  ///< Per part: its texture asset, or none.
        contract::renderer::StaticMeshId mesh;                          ///< The renderer's id, once loaded.
        std::string error;                                              ///< Why it failed to load.
    };

    MeshAssets() = default;
    ~MeshAssets() = default;
    MeshAssets(const MeshAssets& other) = delete;
    MeshAssets& operator=(const MeshAssets& other) = delete;
    MeshAssets(MeshAssets&& other) noexcept = default;
    MeshAssets& operator=(MeshAssets&& other) noexcept = default;

    /// @brief Sets where the files are read: the game's folder or its pack (plugins::assetSourceOf()).
    void setSource(std::shared_ptr<const core::vfs::FileSource> source);
    /// @return Where the files are read; null before setSource().
    [[nodiscard]] const core::vfs::FileSource* getSource() const noexcept;

    /// @brief Asks for the glTF 2.0 file at @p path (`.gltf` or `.glb`), read by the plugin from the next frame.
    /// @return Its id: the same for the same path, in every process.
    core::asset::AssetId load(const std::filesystem::path& path);

    /// @return The entry of @p id, or null if it was never asked for.
    [[nodiscard]] const Entry* get(core::asset::AssetId id) const;
    /// @return The entry of @p id to change, for the plugin; null if it was never asked for.
    [[nodiscard]] Entry* getMut(core::asset::AssetId id);
    /// @return The renderer's mesh of @p id once it is loaded; nothing before, or if it failed.
    [[nodiscard]] std::optional<contract::renderer::StaticMeshId> getMesh(core::asset::AssetId id) const;
    /// @return The ids of every entry, for the plugin's systems.
    [[nodiscard]] std::vector<core::asset::AssetId> getIds() const;

  private:
    std::shared_ptr<const core::vfs::FileSource> _source;      ///< Where files are read.
    std::unordered_map<core::asset::AssetId, Entry> _entries;  ///< By id.
};

AYLO_ECS_RESOURCE(MeshAssets, "aylo.MeshAssets");
}  // namespace aylo::plugins::mesh
