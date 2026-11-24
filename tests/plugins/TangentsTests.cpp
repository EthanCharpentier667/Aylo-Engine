/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TangentsTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float4.hpp>
#include <vector>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "plugins/mesh/detail/Tangents.hpp"

using aylo::contract::renderer::StaticMeshDesc;
using aylo::contract::renderer::StaticMeshVertex;

namespace {
/// @return A square facing +Z, U along @p uAxis's sign of X.
StaticMeshDesc square(float uAlongX) {
    return StaticMeshDesc{.vertices = {StaticMeshVertex{.position = {0, 0, 0}, .normal = {0, 0, 1}, .uv = {0, 0}},
                                       StaticMeshVertex{.position = {1, 0, 0}, .normal = {0, 0, 1}, .uv = {uAlongX, 0}},
                                       StaticMeshVertex{.position = {1, 1, 0}, .normal = {0, 0, 1}, .uv = {uAlongX, 1}},
                                       StaticMeshVertex{.position = {0, 1, 0}, .normal = {0, 0, 1}, .uv = {0, 1}}},
                          .indices = {0, 1, 2, 0, 2, 3}};
}
}  // namespace

TEST(Tangents, PointAlongUAndKeepTheSignOfAMirroredTexture) {
    StaticMeshDesc mesh = square(1.0F);
    aylo::plugins::mesh::detail::generateMissingTangents(mesh, std::vector<bool>(4, false));
    ASSERT_EQ(mesh.tangents.size(), 4U);
    for (const glm::vec4& tangent : mesh.tangents) {
        EXPECT_NEAR(tangent.x, 1.0F, 1e-4F);
        EXPECT_NEAR(tangent.w, 1.0F, 1e-4F);
    }
    StaticMeshDesc mirrored = square(-1.0F);
    aylo::plugins::mesh::detail::generateMissingTangents(mirrored, std::vector<bool>(4, false));
    EXPECT_NEAR(mirrored.tangents.at(0).x, -1.0F, 1e-4F);
}

TEST(Tangents, KeepTheOnesTheFileGave) {
    StaticMeshDesc mesh = square(1.0F);
    mesh.tangents = std::vector<glm::vec4>(4, glm::vec4{0.0F, 1.0F, 0.0F, 1.0F});
    aylo::plugins::mesh::detail::generateMissingTangents(mesh, {true, true, false, true});
    EXPECT_NEAR(mesh.tangents.at(0).y, 1.0F, 1e-4F);
    EXPECT_NEAR(mesh.tangents.at(2).x, 1.0F, 1e-4F);  // the one missing, made
}
