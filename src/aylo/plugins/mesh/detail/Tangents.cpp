/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Tangents
*/

#include "plugins/mesh/detail/Tangents.hpp"

#include <mikktspace.h>

#include <algorithm>
#include <cstddef>
#include <glm/ext/vector_float4.hpp>
#include <span>
#include <vector>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"

namespace aylo::plugins::mesh::detail {

namespace {

/// @brief What MikkTSpace's callbacks read and write: the mesh, and the tangents it gives.
struct Generation {
    const contract::renderer::StaticMeshDesc* mesh;  ///< Read.
    std::vector<glm::vec4> tangents;                 ///< One per vertex, written.
};

/// @return The generation MikkTSpace hands its callbacks.
Generation& generationOf(const SMikkTSpaceContext* context) { return *static_cast<Generation*>(context->m_pUserData); }

/// @return The vertex of corner @p corner of face @p face.
const contract::renderer::StaticMeshVertex& vertexOf(const SMikkTSpaceContext* context, int face, int corner) {
    const contract::renderer::StaticMeshDesc& mesh = *generationOf(context).mesh;
    return mesh.vertices.at(mesh.indices.at((static_cast<std::size_t>(face) * 3) + static_cast<std::size_t>(corner)));
}

int faces(const SMikkTSpaceContext* context) {
    return static_cast<int>(generationOf(context).mesh->indices.size() / 3);
}

int cornersOf(const SMikkTSpaceContext* /*context*/, int /*face*/) { return 3; }

void position(const SMikkTSpaceContext* context, float* out, int face, int corner) {
    const contract::renderer::StaticMeshVertex& vertex = vertexOf(context, face, corner);
    std::ranges::copy(std::span{&vertex.position.x, 3}, std::span{out, 3}.begin());
}

void normal(const SMikkTSpaceContext* context, float* out, int face, int corner) {
    const contract::renderer::StaticMeshVertex& vertex = vertexOf(context, face, corner);
    std::ranges::copy(std::span{&vertex.normal.x, 3}, std::span{out, 3}.begin());
}

void texcoord(const SMikkTSpaceContext* context, float* out, int face, int corner) {
    const contract::renderer::StaticMeshVertex& vertex = vertexOf(context, face, corner);
    std::ranges::copy(std::span{&vertex.uv.x, 2}, std::span{out, 2}.begin());
}

void setTangent(const SMikkTSpaceContext* context, const float* tangent, float sign, int face, int corner) {
    Generation& generation = generationOf(context);
    const std::span<const float> values{tangent, 3};
    // A vertex shared by several faces gets the last one's: the faces of a welded vertex agree on it.
    glm::vec4& written = generation.tangents.at(
        generation.mesh->indices.at((static_cast<std::size_t>(face) * 3) + static_cast<std::size_t>(corner)));
    std::ranges::copy(values, &written.x);
    written.w = sign;
}

}  // namespace

void generateMissingTangents(contract::renderer::StaticMeshDesc& mesh, const std::vector<bool>& known) {
    const std::size_t count = mesh.vertices.size();
    mesh.tangents.resize(count, glm::vec4{1.0F, 0.0F, 0.0F, 1.0F});
    if (std::ranges::all_of(known, [](bool has) { return has; }) && known.size() == count) {
        return;
    }
    Generation generation{.mesh = &mesh, .tangents = mesh.tangents};
    SMikkTSpaceInterface callbacks{};
    callbacks.m_getNumFaces = &faces;
    callbacks.m_getNumVerticesOfFace = &cornersOf;
    callbacks.m_getPosition = &position;
    callbacks.m_getNormal = &normal;
    callbacks.m_getTexCoord = &texcoord;
    callbacks.m_setTSpaceBasic = &setTangent;
    const SMikkTSpaceContext context{.m_pInterface = &callbacks, .m_pUserData = &generation};
    if (genTangSpaceDefault(&context) == 0) {
        return;  // out of memory: the default tangents stay
    }
    for (std::size_t vertex = 0; vertex < count; ++vertex) {
        if (vertex >= known.size() || !known.at(vertex)) {
            mesh.tangents.at(vertex) = generation.tangents.at(vertex);
        }
    }
}

}  // namespace aylo::plugins::mesh::detail
