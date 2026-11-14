/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ModelLoader
*/

#pragma once

#include <string_view>

#include "Model.hpp"
#include "core/vfs/FileSource.hpp"
#include "plugins/mesh/Export.hpp"

namespace aylo::plugins::mesh::detail {

/// @return Whether @p path names a model format the plugin reads, by its extension (any case): `.gltf`, `.glb`,
/// `.fbx`, `.obj`.
[[nodiscard]] AYLO_PLUGIN_MESH_API bool isModelFile(std::string_view path);

/// @return The model of the file at @p path, read through @p source by the loader of its extension: loadGltf() for
/// `.gltf` and `.glb`, loadUfbx() for `.fbx` and `.obj`.
/// @throws core::asset::exceptions::AssetLoadException If the format is not one of those, or the file cannot be read,
/// naming the file and why.
[[nodiscard]] AYLO_PLUGIN_MESH_API Model loadModel(std::string_view path, const core::vfs::FileSource& source);

}  // namespace aylo::plugins::mesh::detail
