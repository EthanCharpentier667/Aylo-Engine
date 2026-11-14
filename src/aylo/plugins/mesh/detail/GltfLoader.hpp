/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GltfLoader
*/

#pragma once

#include <string_view>

#include "Model.hpp"
#include "core/vfs/FileSource.hpp"
#include "plugins/mesh/Export.hpp"

namespace aylo::plugins::mesh::detail {

/// @return The model of the glTF 2.0 file at @p path (`.gltf` with its `.bin` and images, or `.glb`), read through
/// @p source: every triangle primitive of the default scene's nodes, placed by the node's transform, with its base
/// colour factor and texture. Without a scene, every mesh at the origin.
/// @throws core::asset::exceptions::AssetLoadException If the file cannot be read or is not a glTF 2.0 model with
/// triangles, naming the file and why.
[[nodiscard]] AYLO_PLUGIN_MESH_API Model loadGltf(std::string_view path, const core::vfs::FileSource& source);

}  // namespace aylo::plugins::mesh::detail
