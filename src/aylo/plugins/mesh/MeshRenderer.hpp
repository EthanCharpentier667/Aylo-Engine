/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshRenderer
*/

#pragma once

#include <glm/ext/vector_float4.hpp>
#include <vector>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep

namespace aylo::plugins::mesh {

/// @brief The component ("aylo.MeshRenderer") that draws a 3D model at its entity's GlobalTransform3D, through the 3D
/// camera, unlit, or with a material of its own.
/// @details The mesh is an asset id from MeshAssets::load() (`engine.assets.mesh` in a script); it is drawn once it is
/// loaded. The material, from MaterialAssets (`engine.assets.material` in a script), must have a `#pragma mesh` shader;
/// until it is created, or if its shader is a 2D one, the model is drawn with the built-in unlit shader. Client-side:
/// the server has no mesh.
/// @code
/// commands.spawn()
///     .add(aylo::plugins::transform::Transform3D{})
///     .add(aylo::plugins::mesh::MeshRenderer{.mesh = meshes.load("models/crate.glb")});
/// @endcode
struct MeshRenderer {
    core::asset::AssetId mesh;  ///< The model, from MeshAssets::load().
    glm::vec4 color{1.0F};      ///< Linear RGBA, not premultiplied: multiplies the model's colours (a tint, a fade).
    core::asset::AssetId material;  ///< A material of a `#pragma mesh` shader; none: the built-in unlit shader.
    std::vector<core::asset::AssetId> materials;  ///< Per part (a glTF primitive, an FBX or OBJ material), replacing
                                                  ///< `material`; a missing or empty one keeps it.
};

AYLO_ECS_COMPONENT(MeshRenderer, "aylo.MeshRenderer", mesh, color, material, materials);
}  // namespace aylo::plugins::mesh
