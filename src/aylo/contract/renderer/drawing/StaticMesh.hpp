/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StaticMesh
*/

#pragma once

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <vector>

#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"

namespace aylo::contract::renderer {

/// @brief A 3D model uploaded to a renderer (IRenderer::createStaticMesh()), by slot index and generation, as a
/// TextureId. Local to one renderer.
struct StaticMeshId {
    std::uint32_t index{0};       ///< Slot in the renderer's table.
    std::uint32_t generation{0};  ///< Version of the slot; 0 means "no mesh".

    [[nodiscard]] bool operator==(const StaticMeshId& other) const = default;
};

/// @brief One vertex of a static mesh: 32 bytes.
struct StaticMeshVertex {
    glm::vec3 position{0.0F};  ///< In the mesh's own axes (right-handed, Y up).
    glm::vec3 normal{0.0F};    ///< The surface's direction, of length 1 (kept for lighting; unlit drawing ignores it).
    glm::vec2 uv{0.0F};        ///< Texture coordinates, in [0, 1] over the texture, from its top-left corner.
};

/// @brief A run of a static mesh's indices drawn with one material: a glTF primitive.
struct StaticMeshPart {
    std::uint32_t indexOffset{0};  ///< First index of the run.
    std::uint32_t indexCount{0};   ///< Number of indices: a multiple of 3, one triangle per 3.
    Color baseColor{1.0F};         ///< Linear RGBA, not premultiplied: multiplies the texture.
    TextureId texture;             ///< The base colour texture; none: plain baseColor.
};

/// @brief What IRenderer::createStaticMesh() uploads: indexed triangles and their parts. The textures of the parts must
/// exist when the mesh is drawn; destroying a texture does not destroy the mesh.
struct StaticMeshDesc {
    std::vector<StaticMeshVertex> vertices;  ///< Every vertex.
    std::vector<std::uint32_t> indices;      ///< Indices into vertices, three per triangle.
    std::vector<StaticMeshPart> parts;       ///< The runs of indices, each with its material.
};

/// @brief One drawing of a static mesh in the 3D world, by IRenderer::drawStaticMesh().
struct StaticMeshDraw {
    StaticMeshId mesh;      ///< What to draw.
    glm::mat4 world{1.0F};  ///< Places the mesh in the world.
    Color color{1.0F};      ///< Linear RGBA, not premultiplied: multiplies every part's colour (a tint, a fade).
};

}  // namespace aylo::contract::renderer
