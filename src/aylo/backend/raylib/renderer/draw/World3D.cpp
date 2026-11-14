/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** World3D
*/

#include "World3D.hpp"

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/matrix.hpp>
#include <iterator>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "aylo/contract/renderer/drawing/Lines3D.hpp"
#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/view/CameraView.hpp"
#include "aylo/contract/renderer/view/CameraView3D.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"
#include "backend/raylib/renderer/Convert.hpp"
#include "backend/raylib/renderer/TextureTable.hpp"

namespace aylo::backend::raylib::renderer::draw {
namespace {
namespace cr = contract::renderer;

/// @return @p count floats, allocated by raylib, which UnloadMesh() frees.
float* allocateFloats(std::size_t count) {
    return static_cast<float*>(MemAlloc(static_cast<unsigned int>(count * sizeof(float))));
}

/// @return The triangles of @p part as an unindexed raylib mesh, uploaded.
::Mesh uploadPart(const cr::StaticMeshDesc& desc, const cr::StaticMeshPart& part) {
    const std::size_t count = part.indexCount - (part.indexCount % 3);
    ::Mesh mesh{};
    mesh.vertexCount = static_cast<int>(count);
    mesh.triangleCount = static_cast<int>(count / 3);
    mesh.vertices = allocateFloats(count * 3);
    mesh.normals = allocateFloats(count * 3);
    mesh.texcoords = allocateFloats(count * 2);
    const std::span<float> positions{mesh.vertices, count * 3};
    const std::span<float> normals{mesh.normals, count * 3};
    const std::span<float> uvs{mesh.texcoords, count * 2};
    for (std::size_t corner = 0; corner < count; ++corner) {
        const cr::StaticMeshVertex& vertex = desc.vertices.at(desc.indices.at(part.indexOffset + corner));
        const auto at = [corner](const std::span<float> values, std::size_t size) {
            return std::next(values.begin(), static_cast<std::ptrdiff_t>(corner * size));
        };
        std::ranges::copy(std::array{vertex.position.x, vertex.position.y, vertex.position.z}, at(positions, 3));
        std::ranges::copy(std::array{vertex.normal.x, vertex.normal.y, vertex.normal.z}, at(normals, 3));
        std::ranges::copy(std::array{vertex.uv.x, vertex.uv.y}, at(uvs, 2));
    }
    UploadMesh(&mesh, false);
    return mesh;
}

/// @throws std::invalid_argument If an index of @p desc names no vertex, or a part goes past its indices.
void validate(const cr::StaticMeshDesc& desc) {
    if (desc.vertices.empty() || desc.indices.empty()) {
        throw std::invalid_argument("A static mesh needs vertices and indices");
    }
    for (const std::uint32_t index : desc.indices) {
        if (index >= desc.vertices.size()) {
            throw std::invalid_argument(
                std::format("A static mesh index ({}) names no vertex (it has {})", index, desc.vertices.size()));
        }
    }
    for (const cr::StaticMeshPart& part : desc.parts) {
        if (static_cast<std::size_t>(part.indexOffset) + part.indexCount > desc.indices.size()) {
            throw std::invalid_argument("A static mesh part goes past its indices");
        }
    }
}

/// @return @p matrix as a raylib matrix, whose fields go row by row.
::Matrix toMatrix(const glm::mat4& matrix) {
    static_assert(sizeof(::Matrix) == sizeof(glm::mat4), "raylib's Matrix must be 16 floats");
    return std::bit_cast<::Matrix>(glm::transpose(matrix));
}

/// @return raylib's 1x1 white texture.
::Texture2D whiteTexture() {
    return ::Texture2D{.id = rlGetTextureIdDefault(),
                       .width = 1,
                       .height = 1,
                       .mipmaps = 1,
                       .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
}
}  // namespace

World3D::~World3D() {
    if (!IsWindowReady()) {
        return;
    }
    for (const std::optional<std::vector<Part>>& parts : _meshes) {
        if (parts) {
            for (const Part& part : *parts) {
                UnloadMesh(part.mesh);
            }
        }
    }
    if (_material) {
        MemFree(_material->maps);  // the default material's shader and texture are raylib's own
    }
}

cr::StaticMeshId World3D::create(const cr::StaticMeshDesc& desc) {
    validate(desc);
    std::vector<cr::StaticMeshPart> wanted = desc.parts;
    if (wanted.empty()) {
        wanted.push_back(
            cr::StaticMeshPart{.indexOffset = 0, .indexCount = static_cast<std::uint32_t>(desc.indices.size())});
    }
    std::vector<Part> parts;
    for (const cr::StaticMeshPart& part : wanted) {
        if (part.indexCount < 3) {
            continue;
        }
        parts.push_back(Part{.mesh = uploadPart(desc, part), .baseColor = part.baseColor, .texture = part.texture});
    }
    std::uint32_t index = 0;
    if (!_freeSlots.empty()) {
        index = _freeSlots.back();
        _freeSlots.pop_back();
        _meshes.at(index) = std::move(parts);
    } else {
        index = static_cast<std::uint32_t>(_meshes.size());
        _meshes.emplace_back(std::move(parts));
        _generations.push_back(0);
    }
    return {.index = index, .generation = ++_generations.at(index)};
}

void World3D::destroy(cr::StaticMeshId id) {
    const std::vector<Part>* parts = find(id);
    if (parts == nullptr) {
        return;
    }
    for (const Part& part : *parts) {
        UnloadMesh(part.mesh);
    }
    _meshes.at(id.index).reset();
    ++_generations.at(id.index);
    _freeSlots.push_back(id.index);
}

const std::vector<World3D::Part>* World3D::find(cr::StaticMeshId id) const {
    if (id.generation == 0 || id.index >= _meshes.size() || _generations.at(id.index) != id.generation) {
        return nullptr;
    }
    const std::optional<std::vector<Part>>& parts = _meshes.at(id.index);
    return parts.has_value() ? &parts.value() : nullptr;
}

void World3D::begin() {
    _view.reset();
    _segments.clear();
    _draws.clear();
}

void World3D::setView(const cr::View3D& view, glm::vec2 viewSize, glm::uvec2 framebuffer) {
    _view.emplace(view, viewSize, framebuffer);
}

void World3D::add(const cr::Lines3D& lines) {
    for (std::size_t index = 0; index + 1 < lines.points.size(); index += 2) {
        _segments.push_back(Segment{.from = glm::vec3{lines.world * glm::vec4{lines.points.at(index), 1.0F}},
                                    .to = glm::vec3{lines.world * glm::vec4{lines.points.at(index + 1), 1.0F}},
                                    .color = lines.color});
    }
}

void World3D::add(const cr::StaticMeshDraw& draw) { _draws.push_back(draw); }

void World3D::drawMeshes(const TextureTable& textures) {
    if (!_material) {
        _material = LoadMaterialDefault();
    }
    ::MaterialMap& diffuse = _material->maps[MATERIAL_MAP_DIFFUSE];  // NOLINT(*-pro-bounds-pointer-arithmetic)
    for (const cr::StaticMeshDraw& draw : _draws) {
        const std::vector<Part>* parts = find(draw.mesh);
        if (parts == nullptr) {
            continue;
        }
        const ::Matrix transform = toMatrix(draw.world);
        for (const Part& part : *parts) {
            const ::Texture2D* texture = textures.find(part.texture);
            diffuse.texture = texture != nullptr ? *texture : whiteTexture();
            diffuse.color = toColor(part.baseColor * draw.color);
            DrawMesh(part.mesh, *_material, transform);
        }
    }
}

void World3D::draw(const TextureTable& textures, glm::uvec2 framebuffer) {
    if (!_view || (_segments.empty() && _draws.empty())) {
        _segments.clear();
        _draws.clear();
        return;
    }
    const cr::Viewport& viewport = _view->getViewport();
    rlDrawRenderBatchActive();
    rlViewport(static_cast<int>(viewport.position.x),
               static_cast<int>(static_cast<float>(framebuffer.y) - viewport.position.y - viewport.size.y),
               static_cast<int>(viewport.size.x), static_cast<int>(viewport.size.y));
    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();
    rlMultMatrixf(glm::value_ptr(_view->getOpenGLProjection()));
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    rlMultMatrixf(glm::value_ptr(_view->getViewMatrix()));
    rlEnableDepthTest();
    rlDisableBackfaceCulling();  // as the Vulkan renderer: both faces of every triangle

    drawMeshes(textures);
    for (const Segment& segment : _segments) {
        DrawLine3D(::Vector3{.x = segment.from.x, .y = segment.from.y, .z = segment.from.z},
                   ::Vector3{.x = segment.to.x, .y = segment.to.y, .z = segment.to.z}, toColor(segment.color));
    }

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    rlDisableDepthTest();
    rlMatrixMode(RL_PROJECTION);
    rlPopMatrix();
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    rlViewport(0, 0, static_cast<int>(framebuffer.x), static_cast<int>(framebuffer.y));
    _segments.clear();
    _draws.clear();
}

}  // namespace aylo::backend::raylib::renderer::draw
