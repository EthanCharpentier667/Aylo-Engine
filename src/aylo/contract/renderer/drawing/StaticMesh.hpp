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
#include <glm/ext/vector_float4.hpp>
#include <span>
#include <vector>

#include "aylo/contract/renderer/Export.hpp"
#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"

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

/// @brief How a part of a model lets what is behind it through (glTF's alphaMode).
enum class AlphaMode : std::uint8_t {
    kOpaque,  ///< Not at all: the alpha is ignored.
    kMask,    ///< Cut out: a pixel whose alpha is below the part's alphaCutoff is not drawn, the others are opaque.
    kBlend,   ///< Blended by its alpha, drawn after the opaque parts, back to front.
};

/// @brief A run of a static mesh's indices drawn with one material: a glTF primitive, an FBX or OBJ material's faces.
/// Its material is glTF's metallic-roughness: what the built-in material draws it with, unlit (base colour and texture
/// only) or lit (every map). A texture of data (metallic-roughness, normal, occlusion) must be created linear
/// (TextureEncoding::kLinear); the colour ones (base colour, emissive) sRGB.
struct StaticMeshPart {
    std::uint32_t indexOffset{0};  ///< First index of the run.
    std::uint32_t indexCount{0};   ///< Number of indices: a multiple of 3, one triangle per 3.
    Color baseColor{1.0F};         ///< Linear RGBA, not premultiplied: multiplies the texture.
    TextureId texture;             ///< The base colour texture; none: plain baseColor.
    float metallic{0.0F};   ///< 0: dielectric, 1: metal; multiplies the blue channel of metallicRoughnessTexture.
    float roughness{0.5F};  ///< 0: mirror, 1: rough; multiplies its green channel.
    TextureId metallicRoughnessTexture;       ///< Roughness in green, metallic in blue (glTF); none: the factors alone.
    TextureId normalTexture;                  ///< A tangent-space normal map; none: the vertices' normals.
    float normalScale{1.0F};                  ///< Scales the normal map's X and Y.
    TextureId occlusionTexture;               ///< Ambient occlusion in red; none: no occlusion.
    float occlusionStrength{1.0F};            ///< How much of the occlusion applies.
    glm::vec3 emissive{0.0F};                 ///< Linear RGB light the part gives off, its strength applied.
    TextureId emissiveTexture;                ///< Multiplies emissive; none: emissive alone.
    AlphaMode alphaMode{AlphaMode::kOpaque};  ///< How it lets what is behind through.
    float alphaCutoff{0.5F};                  ///< kMask: the alpha below which a pixel is not drawn.
    bool doubleSided{true};  ///< Both faces drawn and lit; false: back faces culled (glTF's default is false; a part
                             ///< made in code keeps both, as before).
    glm::vec4 uvTransform{0.0F, 0.0F, 1.0F, 1.0F};  ///< Offset (xy) and scale (zw) of every texture's coordinates.
    float uvRotation{0.0F};                         ///< And their rotation, radians (KHR_texture_transform).
};

/// @brief The box a static mesh fits in, in its own axes: what a 3D shader reads as AABB_MIN and AABB_MAX.
struct StaticMeshBounds {
    glm::vec3 min{0.0F};  ///< The smallest x, y and z of its vertices.
    glm::vec3 max{0.0F};  ///< The largest x, y and z of its vertices.

    [[nodiscard]] bool operator==(const StaticMeshBounds& other) const = default;
};

/// @brief What IRenderer::createStaticMesh() uploads: indexed triangles and their parts. The textures of the parts must
/// exist when the mesh is drawn; destroying a texture does not destroy the mesh.
struct StaticMeshDesc {
    std::vector<StaticMeshVertex> vertices;  ///< Every vertex.
    std::vector<std::uint32_t> indices;      ///< Indices into vertices, three per triangle.
    std::vector<StaticMeshPart> parts;       ///< The runs of indices, each with its material.
    /// Per vertex, or empty: xyz the tangent (unit, along U), w the sign of the bitangent (±1). Needed by normal maps.
    std::vector<glm::vec4> tangents;
    std::vector<glm::vec4> colors;  ///< Per vertex, or empty (white): linear RGBA multiplying the base colour.
    std::vector<glm::vec2> uv2;     ///< Per vertex, or empty: a second set of texture coordinates (lightmaps).
};

/// @brief One drawing of a static mesh in the 3D world, by IRenderer::drawStaticMesh().
struct StaticMeshDraw {
    StaticMeshId mesh;      ///< What to draw.
    glm::mat4 world{1.0F};  ///< Places the mesh in the world.
    Color color{1.0F};      ///< Linear RGBA, not premultiplied: multiplies every part's colour (a tint, a fade).
    MaterialId material;  ///< A material whose shader has a ShaderTarget::kMesh variant; the default: built-in, unlit.
    ShaderInstanceValues instanceValues{};  ///< The materials' `instance uniform` values for this draw (its entity's).
    /// Per part, in the order of StaticMeshDesc::parts: its material, replacing `material`; a default MaterialId, or a
    /// part past the end, keeps `material`. Only read during drawStaticMesh().
    std::span<const MaterialId> partMaterials;
    /// Draws only the parts partMaterials names (not default): a pass drawn again over some parts (a material's next
    /// pass), the others left alone.
    bool listedPartsOnly{false};
};

/// @return The box @p desc's vertices fit in; zeros for a mesh without vertices.
[[nodiscard]] AYLO_CONTRACT_RENDERER_API StaticMeshBounds computeBounds(const StaticMeshDesc& desc) noexcept;

}  // namespace aylo::contract::renderer
