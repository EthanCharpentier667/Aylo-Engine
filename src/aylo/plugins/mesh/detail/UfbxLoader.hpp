/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** UfbxLoader
*/

#pragma once

#include <string_view>

#include "Model.hpp"
#include "core/vfs/FileSource.hpp"
#include "plugins/mesh/Export.hpp"

namespace aylo::plugins::mesh::detail {

/// @return The model of the FBX (binary or ASCII) or Wavefront OBJ file at @p path, read through @p source with its
/// `.mtl` and images: every mesh of its nodes, placed by the node's transform, its polygons cut into triangles, one
/// part per material with its base colour and texture. Converted to the engine's axes and unit (right-handed, Y up,
/// metres); an OBJ, which names neither, is read as already in them.
/// @throws core::asset::exceptions::AssetLoadException If the file cannot be read or is not a model with triangles,
/// naming the file and why.
[[nodiscard]] AYLO_PLUGIN_MESH_API Model loadUfbx(std::string_view path, const core::vfs::FileSource& source);

}  // namespace aylo::plugins::mesh::detail
