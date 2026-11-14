/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GltfLoader
*/

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "core/vfs/FileSource.hpp"
#include "plugins/mesh/Export.hpp"

namespace aylo::plugins::mesh::detail {

/// @brief An image a glTF file uses: a file next to it, or bytes inside it (a `.glb`, a data URI).
struct GltfImage {
    std::string path;                ///< The image file, from the assets folder; empty for one inside the file.
    std::vector<std::byte> encoded;  ///< The encoded image (PNG, JPEG), for one inside the file.
};

/// @brief What a glTF file holds for the renderer: one mesh, its nodes flattened, and the images of its parts.
struct GltfModel {
    contract::renderer::StaticMeshDesc mesh;  ///< The vertices, indices and parts; the parts' textures unset.
    std::vector<std::optional<std::size_t>> partImages;  ///< Per part: its base colour image in `images`, or none.
    std::vector<GltfImage> images;                       ///< The images the parts use.
};

/// @return The model of the glTF 2.0 file at @p path (`.gltf` with its `.bin` and images, or `.glb`), read through
/// @p source: every triangle primitive of the default scene's nodes, placed by the node's transform, with its base
/// colour factor and texture. Without a scene, every mesh at the origin.
/// @throws core::asset::exceptions::AssetLoadException If the file cannot be read or is not a glTF 2.0 model with
/// triangles, naming the file and why.
[[nodiscard]] AYLO_PLUGIN_MESH_API GltfModel loadGltf(std::string_view path, const core::vfs::FileSource& source);

}  // namespace aylo::plugins::mesh::detail
