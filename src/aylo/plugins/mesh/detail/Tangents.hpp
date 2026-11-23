/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Tangents
*/

#pragma once

#include <vector>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "plugins/mesh/Export.hpp"

namespace aylo::plugins::mesh::detail {

/// @brief Gives @p mesh a tangent for every vertex @p known says has none, with MikkTSpace, the tangent space normal
/// maps are baked in by Blender, Substance and most tools; the vertices that had one keep it. Does nothing to a mesh
/// whose vertices all have one; a mesh with no texture coordinates gets tangents along X.
/// @param known One per vertex: whether its tangent (in mesh.tangents, then filled to one per vertex) came from the
/// file.
AYLO_PLUGIN_MESH_API void generateMissingTangents(contract::renderer::StaticMeshDesc& mesh,
                                                  const std::vector<bool>& known);

}  // namespace aylo::plugins::mesh::detail
