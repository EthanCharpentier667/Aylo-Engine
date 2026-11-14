/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshCalls
*/

#pragma once

#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/type/String.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/mesh/Export.hpp"

namespace aylo::plugins::mesh {

inline constexpr std::string_view kLoadMeshCall = "aylo.mesh.load";  ///< MeshAssets::load().

/// @brief The arguments of `aylo.mesh.load`: asks for a glTF file.
struct LoadMeshArguments {
    ecs::String path;  ///< The `.gltf` or `.glb` file, from the assets folder.
};
AYLO_ECS_CALL_DATA(LoadMeshArguments, "aylo.mesh.LoadMeshArguments", path);

/// @brief The result of `aylo.mesh.load`.
struct MeshResult {
    core::asset::AssetId mesh;  ///< The mesh's id, for a MeshRenderer.
};
AYLO_ECS_CALL_DATA(MeshResult, "aylo.mesh.MeshResult", mesh);

/// @brief Registers `aylo.mesh.load` on @p app, on the `aylo.MeshAssets` resource, and the plugin's setup forms.
/// Called by the MeshPlugin.
AYLO_PLUGIN_MESH_API void addMeshCalls(ecs::App& app);

/// @brief Registers the setup form `engine.assets.mesh` on @p registry: its library does it on every side, so a server
/// gets the ids of the meshes it does not load.
AYLO_PLUGIN_MESH_API void addMeshForms(ecs::PluginRegistry& registry);

}  // namespace aylo::plugins::mesh
