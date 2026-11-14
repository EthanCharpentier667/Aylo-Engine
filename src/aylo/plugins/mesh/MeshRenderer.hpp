/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshRenderer
*/

#pragma once

#include <glm/ext/vector_float4.hpp>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep

namespace aylo::plugins::mesh {

/// @brief The component ("aylo.MeshRenderer") that draws a 3D model at its entity's GlobalTransform3D, through the 3D
/// camera, unlit.
/// @details The mesh is an asset id from MeshAssets::load() (`engine.assets.mesh` in a script); it is drawn once it is
/// loaded. Client-side: the server has no mesh.
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform3D{})
///     .add(aylo::plugins::mesh::MeshRenderer{.mesh = meshes.load("models/crate.glb")});
/// @endcode
struct MeshRenderer {
    core::asset::AssetId mesh;  ///< The model, from MeshAssets::load().
    glm::vec4 color{1.0F};      ///< Linear RGBA, not premultiplied: multiplies the model's colours (a tint, a fade).
};

AYLO_ECS_COMPONENT(MeshRenderer, "aylo.MeshRenderer", mesh, color);
}  // namespace aylo::plugins::mesh
