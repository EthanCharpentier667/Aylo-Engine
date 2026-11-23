/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** UfbxLoader
*/

// The only file that sees ufbx: nothing of it leaves the plugin.
#include "UfbxLoader.hpp"

#include <ufbx.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Model.hpp"
#include "Tangents.hpp"
#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/vfs/FileSource.hpp"

// ufbx gives its vectors, material values and strings as unions of their views (`x`/`v[0]`...), the documented way to
// read them.
// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
namespace aylo::plugins::mesh::detail {
namespace {

/// @return @p text as a string view.
std::string_view viewOf(ufbx_string text) noexcept { return {text.data, text.length}; }

/// @return @p vector in single precision (ufbx works in double).
glm::vec3 toGlm(ufbx_vec3 vector) noexcept {
    return {static_cast<float>(vector.x), static_cast<float>(vector.y), static_cast<float>(vector.z)};
}

/// @brief What the file callback of ufbx reads through: the game's files.
struct ReadContext {
    const core::vfs::FileSource* source = nullptr;  ///< The game's folder or pack.
};

/// @brief ufbx's file open, for the files a model names (an OBJ's `.mtl`): read through the game's files, by their path
/// from the assets folder. Never throws: ufbx is C.
bool openFile(void* user, ufbx_stream* stream, const char* path, std::size_t length, const ufbx_open_file_info* info) {
    const auto* context = static_cast<const ReadContext*>(user);
    try {
        const std::string bytes = context->source->read(
            std::filesystem::path{std::string_view{path, length}}.lexically_normal().generic_string());
        // Copied by ufbx (no_copy unset), so the bytes may go when this returns.
        const ufbx_open_memory_opts options{};
        return ufbx_open_memory_ctx(stream, info->context, bytes.data(), bytes.size(), &options, nullptr);
    } catch (const std::exception&) {
        return false;
    }
}

/// @brief Frees a ufbx scene.
struct SceneDeleter {
    void operator()(ufbx_scene* scene) const noexcept { ufbx_free_scene(scene); }
};

/// @brief Builds a Model from a loaded scene.
class ModelBuilder {
  public:
    explicit ModelBuilder(std::string_view path) : _path{path} {}

    /// @brief Adds @p node's mesh, if it has one, placed by its geometry-to-world transform.
    /// @throws core::asset::exceptions::AssetLoadException If ufbx cannot index its vertices.
    void addNode(const ufbx_node& node) {
        if (node.mesh == nullptr) {
            return;
        }
        const ufbx_mesh& mesh = *node.mesh;
        const ufbx_matrix normals = ufbx_matrix_for_normals(&node.geometry_to_world);
        std::vector<std::uint32_t> corners(mesh.max_face_triangles * 3);
        for (const ufbx_mesh_part& part : std::span{mesh.material_parts.data, mesh.material_parts.count}) {
            // The node's materials, as an instance may have its own: the same indices as the mesh's.
            const ufbx_material* material =
                part.index < node.materials.count ? node.materials.data[part.index] : nullptr;  // NOLINT
            addPart(mesh, part, node.geometry_to_world, normals, material, corners);
        }
    }

    /// @return The model built.
    /// @throws core::asset::exceptions::AssetLoadException If it has no triangle.
    Model take() {
        if (_model.mesh.indices.empty()) {
            throw core::asset::exceptions::AssetLoadException{
                std::format("Mesh '{}': it has no triangles to draw", _path)};
        }
        // Tangents are only for normal maps: MikkTSpace makes them for a model that has one (FBX files are baked so).
        const bool normalMaps = std::ranges::any_of(_model.partMaps, [](const auto& maps) {
            return maps.at(static_cast<std::size_t>(PartMap::kNormal)).has_value();
        });
        if (normalMaps) {
            generateMissingTangents(_model.mesh, std::vector<bool>(_model.mesh.vertices.size(), false));
        }
        return std::move(_model);
    }

  private:
    /// @return The vertex of @p mesh's corner @p index, placed by @p world (its normal by @p normals). Its UV is turned
    /// to start from the texture's top-left corner, as the contract's: FBX and OBJ start from the bottom-left.
    static contract::renderer::StaticMeshVertex vertexOf(const ufbx_mesh& mesh, std::size_t index,
                                                         const ufbx_matrix& world, const ufbx_matrix& normals) {
        contract::renderer::StaticMeshVertex vertex;
        vertex.position = toGlm(ufbx_transform_position(&world, ufbx_get_vertex_vec3(&mesh.vertex_position, index)));
        if (mesh.vertex_normal.exists) {
            vertex.normal = toGlm(ufbx_vec3_normalize(
                ufbx_transform_direction(&normals, ufbx_get_vertex_vec3(&mesh.vertex_normal, index))));
        }
        if (mesh.vertex_uv.exists) {
            const ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh.vertex_uv, index);
            vertex.uv = glm::vec2{static_cast<float>(uv.x), static_cast<float>(1.0 - uv.y)};
        }
        return vertex;
    }

    /// @brief Adds the faces of @p part, cut into triangles, their vertices shared where they are equal, as one part of
    /// the model with @p material.
    /// @throws core::asset::exceptions::AssetLoadException If ufbx cannot index the vertices.
    void addPart(const ufbx_mesh& mesh, const ufbx_mesh_part& part, const ufbx_matrix& world,
                 const ufbx_matrix& normals, const ufbx_material* material, std::vector<std::uint32_t>& corners) {
        if (part.num_triangles == 0) {
            return;
        }
        std::vector<contract::renderer::StaticMeshVertex> vertices;
        vertices.reserve(part.num_triangles * 3);
        for (const std::uint32_t face : std::span{part.face_indices.data, part.face_indices.count}) {
            const std::uint32_t triangles =
                ufbx_triangulate_face(corners.data(), corners.size(), &mesh, mesh.faces.data[face]);  // NOLINT
            for (std::size_t corner = 0; corner < static_cast<std::size_t>(triangles) * 3; ++corner) {
                vertices.push_back(vertexOf(mesh, corners.at(corner), world, normals));
            }
        }
        // ufbx_generate_indices() compares the vertices byte by byte: StaticMeshVertex has no padding (32 bytes).
        static_assert(sizeof(contract::renderer::StaticMeshVertex) == 32);
        std::vector<std::uint32_t> indices(vertices.size());
        const ufbx_vertex_stream stream{
            .data = vertices.data(), .vertex_count = vertices.size(), .vertex_size = sizeof(vertices.front())};
        ufbx_error error{};
        const std::size_t count = ufbx_generate_indices(&stream, 1, indices.data(), indices.size(), nullptr, &error);
        if (error.type != UFBX_ERROR_NONE) {
            throw core::asset::exceptions::AssetLoadException{
                std::format("Mesh '{}': {}", _path, viewOf(error.description))};
        }
        vertices.resize(count);

        const auto base = static_cast<std::uint32_t>(_model.mesh.vertices.size());
        const auto offset = static_cast<std::uint32_t>(_model.mesh.indices.size());
        _model.mesh.vertices.insert(_model.mesh.vertices.end(), vertices.begin(), vertices.end());
        for (const std::uint32_t index : indices) {
            _model.mesh.indices.push_back(base + index);
        }
        contract::renderer::StaticMeshPart made{.indexOffset = offset,
                                                .indexCount = static_cast<std::uint32_t>(indices.size())};
        const std::optional<std::size_t> image = applyMaterial(material, made);
        _model.mesh.parts.push_back(made);
        _model.partImages.push_back(image);
        _model.partMaps.push_back(mapsOf(material, made));
        _model.partNames.emplace_back(material != nullptr ? viewOf(material->name) : std::string_view{});
    }

    /// @brief Gives @p part the base colour and texture of @p material, if it has them; @return its image.
    std::optional<std::size_t> applyMaterial(const ufbx_material* material, contract::renderer::StaticMeshPart& part) {
        if (material == nullptr) {
            return std::nullopt;
        }
        // ufbx maps every shading model (FBX Phong and Lambert, OBJ's Kd) to these PBR values.
        const ufbx_material_map& color = material->pbr.base_color;
        const ufbx_material_map& factor = material->pbr.base_factor;
        const ufbx_material_map& opacity = material->pbr.opacity;
        if (color.has_value) {
            const auto scale = static_cast<float>(factor.has_value ? factor.value_real : 1.0);
            part.baseColor = {static_cast<float>(color.value_vec4.x) * scale,
                              static_cast<float>(color.value_vec4.y) * scale,
                              static_cast<float>(color.value_vec4.z) * scale,
                              static_cast<float>(opacity.has_value ? opacity.value_real : 1.0)};
        }
        const ufbx_texture* texture = color.texture;
        if (texture == nullptr || !color.texture_enabled) {
            return std::nullopt;
        }
        // A layered or procedural texture: the first image it is made of.
        if (texture->type != UFBX_TEXTURE_FILE) {
            if (texture->file_textures.count == 0) {
                return std::nullopt;
            }
            texture = texture->file_textures.data[0];  // NOLINT(*-pointer-arithmetic)
        }
        return imageOf(*texture);
    }

    /// @return The image of @p map, if it has a texture made of one.
    std::optional<std::size_t> imageOfMap(const ufbx_material_map& map) {
        const ufbx_texture* texture = map.texture;
        if (texture == nullptr || !map.texture_enabled) {
            return std::nullopt;
        }
        if (texture->type != UFBX_TEXTURE_FILE) {
            if (texture->file_textures.count == 0) {
                return std::nullopt;
            }
            texture = texture->file_textures.data[0];  // NOLINT(*-pointer-arithmetic)
        }
        return imageOf(*texture);
    }

    /// @brief Gives @p part @p material's PBR values besides its base colour (ufbx maps every shading model to them):
    /// metalness, roughness, emission, its sides and its opacity.
    /// @return The images of its maps.
    std::array<std::optional<std::size_t>, kPartMapCount> mapsOf(const ufbx_material* material,
                                                                 contract::renderer::StaticMeshPart& part) {
        std::array<std::optional<std::size_t>, kPartMapCount> maps{};
        if (material == nullptr) {
            return maps;
        }
        const ufbx_material_pbr_maps& pbr = material->pbr;
        part.metallic = pbr.metalness.has_value ? static_cast<float>(pbr.metalness.value_real) : 0.0F;
        part.roughness = pbr.roughness.has_value ? static_cast<float>(pbr.roughness.value_real) : 0.5F;
        if (pbr.emission_color.has_value) {
            const auto strength =
                static_cast<float>(pbr.emission_factor.has_value ? pbr.emission_factor.value_real : 1.0);
            part.emissive = glm::vec3{static_cast<float>(pbr.emission_color.value_vec4.x),
                                      static_cast<float>(pbr.emission_color.value_vec4.y),
                                      static_cast<float>(pbr.emission_color.value_vec4.z)} *
                            strength;
        }
        part.doubleSided = !material->features.double_sided.enabled ? part.doubleSided : true;
        if (part.baseColor.w < 1.0F) {
            part.alphaMode = contract::renderer::AlphaMode::kBlend;
        }
        // The roughness and the metalness of an FBX are two images: only a glTF-style packed one is read.
        maps.at(static_cast<std::size_t>(PartMap::kNormal)) = imageOfMap(pbr.normal_map);
        maps.at(static_cast<std::size_t>(PartMap::kOcclusion)) = imageOfMap(pbr.ambient_occlusion);
        maps.at(static_cast<std::size_t>(PartMap::kEmissive)) = imageOfMap(pbr.emission_color);
        return maps;
    }

    /// @return The index of @p texture's image in the model's images, added the first time; none if it names no file.
    std::optional<std::size_t> imageOf(const ufbx_texture& texture) {
        if (const auto known = _images.find(&texture); known != _images.end()) {
            return known->second;
        }
        ModelImage made;
        if (texture.content.size > 0) {
            const std::span<const std::byte> bytes{static_cast<const std::byte*>(texture.content.data),
                                                   texture.content.size};
            made.encoded.assign(bytes.begin(), bytes.end());
        } else {
            // Relative to the model's folder, as the file names it; an absolute path from the machine that exported it
            // is useless here, so then its name alone, next to the model.
            std::string name{viewOf(texture.relative_filename)};
            if (name.empty() || std::filesystem::path{name}.is_absolute()) {
                name = std::filesystem::path{std::string{viewOf(texture.filename)}}.filename().string();
            }
            std::ranges::replace(name, '\\', '/');
            if (name.empty()) {
                return std::nullopt;
            }
            made.path = (std::filesystem::path{_path}.parent_path() / name).lexically_normal().generic_string();
        }
        _model.images.push_back(std::move(made));
        _images.emplace(&texture, _model.images.size() - 1);
        return _model.images.size() - 1;
    }

    std::string _path;                                             ///< For the messages, and to find the images.
    Model _model;                                                  ///< What is built.
    std::unordered_map<const ufbx_texture*, std::size_t> _images;  ///< Images already added, by their index.
};

}  // namespace

Model loadUfbx(std::string_view path, const core::vfs::FileSource& source) {
    std::string bytes;
    try {
        bytes = source.read(path);
    } catch (const core::asset::exceptions::AssetLoadException& error) {
        throw core::asset::exceptions::AssetLoadException{std::format("Mesh '{}': {}", path, error.what())};
    }
    ReadContext context{.source = &source};
    ufbx_load_opts options{};
    // The path names the format when the bytes do not (an OBJ), and where the files it names are.
    options.filename = {.data = path.data(), .length = path.size()};
    options.path_separator = '/';
    options.open_file_cb.fn = &openFile;
    options.open_file_cb.user = &context;
    // An OBJ's .mtl, read only through the game's files; a model whose .mtl is missing still loads, without colours.
    options.load_external_files = true;
    options.ignore_missing_external_files = true;
    options.ignore_animation = true;
    options.generate_missing_normals = true;
    options.target_axes = ufbx_axes_right_handed_y_up;
    options.target_unit_meters = 1.0;
    options.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
    // A deep node tree is a broken or hostile file, never a model.
    options.node_depth_limit = 256;

    ufbx_error error{};
    const std::unique_ptr<ufbx_scene, SceneDeleter> scene{
        ufbx_load_memory(bytes.data(), bytes.size(), &options, &error)};
    if (scene == nullptr) {
        const std::string_view info{std::data(error.info), error.info_length};
        throw core::asset::exceptions::AssetLoadException{std::format(
            "Mesh '{}': {}{}", path, viewOf(error.description), info.empty() ? "" : std::format(" ({})", info))};
    }

    ModelBuilder builder{path};
    for (const ufbx_node* node : std::span{scene->nodes.data, scene->nodes.count}) {
        builder.addNode(*node);
    }
    return builder.take();
}

}  // namespace aylo::plugins::mesh::detail
// NOLINTEND(cppcoreguidelines-pro-type-union-access)
