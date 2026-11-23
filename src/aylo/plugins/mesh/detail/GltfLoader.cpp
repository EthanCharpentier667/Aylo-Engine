/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GltfLoader
*/

// The only file that sees cgltf: its implementation is compiled here, and nothing of it leaves the plugin.
#define CGLTF_IMPLEMENTATION
#include "GltfLoader.hpp"

#include <cgltf.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/matrix.hpp>
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
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/vfs/FileSource.hpp"

namespace aylo::plugins::mesh::detail {
namespace {

/// @brief What the file callbacks of cgltf read through: the game's files.
struct ReadContext {
    const core::vfs::FileSource* source = nullptr;  ///< The game's folder or pack.
    std::string error;                              ///< Why the last read failed.
};

/// @brief cgltf's file read: a buffer (`.bin`) of the model, by its path from the assets folder.
cgltf_result readFile(const cgltf_memory_options* /*memory*/, const cgltf_file_options* file, const char* path,
                      cgltf_size* size, void** data) {
    auto* context = static_cast<ReadContext*>(file->user_data);
    try {
        const std::string bytes =
            context->source->read(std::filesystem::path{path}.lexically_normal().generic_string());
        void* copy = std::malloc(bytes.empty() ? 1 : bytes.size());  // NOLINT(*-no-malloc, *-owning-memory)
        if (copy == nullptr) {
            return cgltf_result_out_of_memory;
        }
        std::ranges::copy(bytes, static_cast<char*>(copy));
        *size = bytes.size();
        *data = copy;
        return cgltf_result_success;
    } catch (const std::exception& error) {
        context->error = error.what();
        return cgltf_result_file_not_found;
    }
}

/// @brief cgltf's file release: frees what readFile() gave.
void releaseFile(const cgltf_memory_options* /*memory*/, const cgltf_file_options* /*file*/, void* data) {
    std::free(data);  // NOLINT(*-no-malloc, *-owning-memory)
}

/// @brief Frees a cgltf model.
struct GltfDeleter {
    void operator()(cgltf_data* data) const noexcept { cgltf_free(data); }
};

/// @return A readable name of a cgltf failure.
std::string_view describe(cgltf_result result) {
    switch (result) {
        case cgltf_result_data_too_short:
            return "the file is cut short";
        case cgltf_result_unknown_format:
            return "it is not a glTF file";
        case cgltf_result_invalid_json:
            return "its JSON is not valid";
        case cgltf_result_invalid_gltf:
            return "it is not a valid glTF 2.0 file";
        case cgltf_result_file_not_found:
            return "a file it names was not found";
        case cgltf_result_io_error:
            return "a file it names could not be read";
        case cgltf_result_out_of_memory:
            return "out of memory";
        case cgltf_result_legacy_gltf:
            return "it is a glTF 1.0 file: only glTF 2.0 is read";
        default:
            return "it could not be read";
    }
}

/// @brief Builds a Model from a parsed file.
class ModelBuilder {
  public:
    ModelBuilder(std::string_view path, const cgltf_options& options) : _path{path}, _options{&options} {}

    /// @brief Adds every triangle primitive of @p mesh, placed by @p world.
    void addMesh(const cgltf_mesh& mesh, const glm::mat4& world) {
        const glm::mat3 normals = glm::transpose(glm::inverse(glm::mat3{world}));
        for (const cgltf_primitive& primitive : std::span{mesh.primitives, mesh.primitives_count}) {
            if (primitive.type == cgltf_primitive_type_triangles) {
                addPrimitive(primitive, world, normals);
            }
        }
    }

    /// @brief Adds @p node's mesh and its children's, from the root down.
    // NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the file's node tree.
    void addNode(const cgltf_node& node) {
        if (node.mesh != nullptr) {
            std::array<cgltf_float, 16> matrix{};
            cgltf_node_transform_world(&node, matrix.data());
            addMesh(*node.mesh, glm::make_mat4(matrix.data()));
        }
        for (const cgltf_node* child : std::span{node.children, node.children_count}) {
            addNode(*child);
        }
    }

    /// @return The model built.
    /// @throws core::asset::exceptions::AssetLoadException If it has no triangle.
    Model take() {
        if (_model.mesh.indices.empty()) {
            throw core::asset::exceptions::AssetLoadException{
                std::format("Mesh '{}': it has no triangles to draw", _path)};
        }
        if (!_anyColor) {
            _model.mesh.colors.clear();
        }
        if (!_anyUv2) {
            _model.mesh.uv2.clear();
        }
        // Tangents are only for normal maps: a model with neither keeps none, and does not pay for them.
        const bool normalMaps = std::ranges::any_of(_model.mesh.parts,
                                                    [](const auto& part) {
                                                        return part.normalTexture != contract::renderer::TextureId{};
                                                    }) ||
                                std::ranges::any_of(_model.partMaps, [](const auto& maps) {
                                    return maps.at(static_cast<std::size_t>(PartMap::kNormal)).has_value();
                                });
        if (_anyTangent || normalMaps) {
            generateMissingTangents(_model.mesh, _knownTangents);
        } else {
            _model.mesh.tangents.clear();
        }
        return std::move(_model);
    }

  private:
    /// @brief The accessors of a primitive's attributes the model uses; null for one it does not have.
    struct Attributes {
        const cgltf_accessor* positions = nullptr;  ///< POSITION.
        const cgltf_accessor* normals = nullptr;    ///< NORMAL.
        const cgltf_accessor* uvs = nullptr;        ///< TEXCOORD_0.
        const cgltf_accessor* tangents = nullptr;   ///< TANGENT.
        const cgltf_accessor* colors = nullptr;     ///< COLOR_0.
        const cgltf_accessor* uv2 = nullptr;        ///< TEXCOORD_1.
    };

    /// @return The attributes of @p primitive the model uses.
    static Attributes attributesOf(const cgltf_primitive& primitive) {
        Attributes found;
        for (const cgltf_attribute& attribute : std::span{primitive.attributes, primitive.attributes_count}) {
            if (attribute.type == cgltf_attribute_type_position) {
                found.positions = attribute.data;
            } else if (attribute.type == cgltf_attribute_type_normal) {
                found.normals = attribute.data;
            } else if (attribute.type == cgltf_attribute_type_texcoord && attribute.index == 0) {
                found.uvs = attribute.data;
            } else if (attribute.type == cgltf_attribute_type_texcoord && attribute.index == 1) {
                found.uv2 = attribute.data;
            } else if (attribute.type == cgltf_attribute_type_tangent) {
                found.tangents = attribute.data;
            } else if (attribute.type == cgltf_attribute_type_color && attribute.index == 0) {
                found.colors = attribute.data;
            }
        }
        return found;
    }

    /// @brief Appends the vertices of @p attributes, placed by @p world (their normals by @p normals).
    void addVertices(const Attributes& attributes, const glm::mat4& world, const glm::mat3& normals) {
        for (cgltf_size index = 0; index < attributes.positions->count; ++index) {
            contract::renderer::StaticMeshVertex vertex;
            glm::vec3 position{0.0F};
            cgltf_accessor_read_float(attributes.positions, index, glm::value_ptr(position), 3);
            vertex.position = glm::vec3{world * glm::vec4{position, 1.0F}};
            if (attributes.normals != nullptr && index < attributes.normals->count) {
                glm::vec3 normal{0.0F};
                cgltf_accessor_read_float(attributes.normals, index, glm::value_ptr(normal), 3);
                const glm::vec3 turned = normals * normal;
                vertex.normal = glm::length(turned) > 0.0F ? glm::normalize(turned) : turned;
            }
            if (attributes.uvs != nullptr && index < attributes.uvs->count) {
                cgltf_accessor_read_float(attributes.uvs, index, glm::value_ptr(vertex.uv), 2);
            }
            _model.mesh.vertices.push_back(vertex);
            addVertexData(attributes, index, world);
        }
    }

    /// @brief Appends the tangent, colour and second UV of vertex @p index of @p attributes, defaults where it has
    /// none: the lists stay one per vertex, and take() keeps only those some vertex has.
    void addVertexData(const Attributes& attributes, cgltf_size index, const glm::mat4& world) {
        glm::vec4 tangent{1.0F, 0.0F, 0.0F, 1.0F};
        const bool hasTangent = attributes.tangents != nullptr && index < attributes.tangents->count;
        if (hasTangent) {
            cgltf_accessor_read_float(attributes.tangents, index, glm::value_ptr(tangent), 4);
            const glm::vec3 turned = glm::mat3{world} * glm::vec3{tangent};
            // A mirroring transform turns the bitangent over.
            const float sign = glm::determinant(glm::mat3{world}) < 0.0F ? -tangent.w : tangent.w;
            tangent = glm::vec4{glm::length(turned) > 0.0F ? glm::normalize(turned) : turned, sign};
        }
        _model.mesh.tangents.push_back(tangent);
        _knownTangents.push_back(hasTangent);
        _anyTangent = _anyTangent || hasTangent;
        glm::vec4 color{1.0F};
        if (attributes.colors != nullptr && index < attributes.colors->count) {
            cgltf_accessor_read_float(attributes.colors, index, glm::value_ptr(color),
                                      cgltf_num_components(attributes.colors->type));
            _anyColor = true;
        }
        _model.mesh.colors.push_back(color);
        glm::vec2 uv2{0.0F};
        if (attributes.uv2 != nullptr && index < attributes.uv2->count) {
            cgltf_accessor_read_float(attributes.uv2, index, glm::value_ptr(uv2), 2);
            _anyUv2 = true;
        }
        _model.mesh.uv2.push_back(uv2);
    }

    /// @brief Appends the indices of @p primitive, shifted by @p base, its vertices numbering @p count; without an
    /// index accessor, the vertices in order.
    /// @throws core::asset::exceptions::AssetLoadException If an index names no vertex.
    void addIndices(const cgltf_primitive& primitive, std::uint32_t base, cgltf_size count) {
        std::vector<std::uint32_t>& indices = _model.mesh.indices;
        if (primitive.indices == nullptr) {
            for (cgltf_size index = 0; index < count; ++index) {
                indices.push_back(base + static_cast<std::uint32_t>(index));
            }
            return;
        }
        for (cgltf_size index = 0; index < primitive.indices->count; ++index) {
            const cgltf_size vertex = cgltf_accessor_read_index(primitive.indices, index);
            if (vertex >= count) {
                throw core::asset::exceptions::AssetLoadException{
                    std::format("Mesh '{}': an index names a vertex it does not have", _path)};
            }
            indices.push_back(base + static_cast<std::uint32_t>(vertex));
        }
    }

    /// @brief Gives @p part @p material: its metallic-roughness factors, alpha mode, sides, emission and texture
    /// transform, and @p maps the images of its other maps. A primitive without a material keeps the part's defaults
    /// (both sides drawn, as before).
    /// @return Its base colour image.
    std::optional<std::size_t> applyMaterial(const cgltf_material* material, contract::renderer::StaticMeshPart& part,
                                             std::array<std::optional<std::size_t>, kPartMapCount>& maps) {
        if (material == nullptr) {
            return std::nullopt;
        }
        part.doubleSided = material->double_sided != 0;
        part.alphaMode = contract::renderer::AlphaMode::kOpaque;
        if (material->alpha_mode == cgltf_alpha_mode_blend) {
            part.alphaMode = contract::renderer::AlphaMode::kBlend;
        } else if (material->alpha_mode == cgltf_alpha_mode_mask) {
            part.alphaMode = contract::renderer::AlphaMode::kMask;
        }
        part.alphaCutoff = material->alpha_cutoff;
        const float strength =
            material->has_emissive_strength != 0 ? material->emissive_strength.emissive_strength : 1.0F;
        part.emissive = glm::make_vec3(static_cast<const cgltf_float*>(material->emissive_factor)) * strength;
        const auto mapOf = [this, &maps](const cgltf_texture_view& view, PartMap map) {
            if (view.texture != nullptr && view.texture->image != nullptr) {
                maps.at(static_cast<std::size_t>(map)) = imageOf(*view.texture->image);
            }
        };
        mapOf(material->normal_texture, PartMap::kNormal);
        part.normalScale = material->normal_texture.scale;
        mapOf(material->occlusion_texture, PartMap::kOcclusion);
        part.occlusionStrength =
            material->occlusion_texture.texture != nullptr ? material->occlusion_texture.scale : 1.0F;
        mapOf(material->emissive_texture, PartMap::kEmissive);
        if (material->has_pbr_metallic_roughness == 0) {
            return std::nullopt;
        }
        const cgltf_pbr_metallic_roughness& pbr = material->pbr_metallic_roughness;
        part.baseColor = glm::make_vec4(static_cast<const cgltf_float*>(pbr.base_color_factor));
        part.metallic = pbr.metallic_factor;
        part.roughness = pbr.roughness_factor;
        mapOf(pbr.metallic_roughness_texture, PartMap::kMetallicRoughness);
        // KHR_texture_transform: the base colour's, applied to every map.
        if (pbr.base_color_texture.has_transform != 0) {
            const cgltf_texture_transform& transform = pbr.base_color_texture.transform;
            part.uvTransform = glm::vec4{transform.offset[0], transform.offset[1], transform.scale[0],
                                         transform.scale[1]};  // NOLINT(*-constant-array-index)
            part.uvRotation = transform.rotation;
        }
        if (pbr.base_color_texture.texture == nullptr || pbr.base_color_texture.texture->image == nullptr) {
            return std::nullopt;
        }
        return imageOf(*pbr.base_color_texture.texture->image);
    }

    /// @brief Adds a triangle primitive: its vertices, its indices (a count not a multiple of 3 cut), and its part.
    void addPrimitive(const cgltf_primitive& primitive, const glm::mat4& world, const glm::mat3& normals) {
        const Attributes attributes = attributesOf(primitive);
        if (attributes.positions == nullptr || attributes.positions->count == 0) {
            return;
        }
        const auto base = static_cast<std::uint32_t>(_model.mesh.vertices.size());
        addVertices(attributes, world, normals);
        const auto offset = static_cast<std::uint32_t>(_model.mesh.indices.size());
        addIndices(primitive, base, attributes.positions->count);
        const auto count = static_cast<std::uint32_t>(_model.mesh.indices.size()) - offset;
        _model.mesh.indices.resize(offset + (count - (count % 3)));
        contract::renderer::StaticMeshPart part{.indexOffset = offset, .indexCount = count - (count % 3)};
        std::array<std::optional<std::size_t>, kPartMapCount> maps{};
        const std::optional<std::size_t> image = applyMaterial(primitive.material, part, maps);
        _model.mesh.parts.push_back(part);
        _model.partImages.push_back(image);
        _model.partMaps.push_back(maps);
        _model.partNames.emplace_back(
            primitive.material != nullptr && primitive.material->name != nullptr ? primitive.material->name : "");
    }

    /// @return The index of @p image in the model's images, added the first time.
    std::optional<std::size_t> imageOf(const cgltf_image& image) {
        if (const auto known = _images.find(&image); known != _images.end()) {
            return known->second;
        }
        ModelImage made;
        if (image.buffer_view != nullptr && image.buffer_view->buffer != nullptr &&
            image.buffer_view->buffer->data != nullptr) {
            const auto* bytes = static_cast<const std::byte*>(image.buffer_view->buffer->data);
            const std::span<const std::byte> view{bytes + image.buffer_view->offset,  // NOLINT(*-pointer-arithmetic)
                                                  image.buffer_view->size};
            made.encoded.assign(view.begin(), view.end());
        } else if (image.uri != nullptr && std::string_view{image.uri}.starts_with("data:")) {
            made.encoded = decodeDataUri(image.uri);
        } else if (image.uri != nullptr) {
            std::string uri{image.uri};
            uri.resize(cgltf_decode_uri(uri.data()));
            made.path = (std::filesystem::path{_path}.parent_path() / uri).lexically_normal().generic_string();
        } else {
            return std::nullopt;
        }
        _model.images.push_back(std::move(made));
        _images.emplace(&image, _model.images.size() - 1);
        return _model.images.size() - 1;
    }

    /// @return The bytes of a base64 data URI (`data:image/png;base64,...`); none for another kind.
    [[nodiscard]] std::vector<std::byte> decodeDataUri(std::string_view uri) const {
        const std::size_t comma = uri.find(";base64,");
        if (comma == std::string_view::npos) {
            return {};
        }
        const std::string base64{uri.substr(comma + std::string_view{";base64,"}.size())};
        std::size_t padding = 0;
        for (auto it = base64.rbegin(); it != base64.rend() && *it == '='; ++it) {
            ++padding;
        }
        const std::size_t size = ((base64.size() * 3) / 4) - padding;
        void* data = nullptr;
        if (cgltf_load_buffer_base64(_options, size, base64.c_str(), &data) != cgltf_result_success) {
            return {};
        }
        const std::span<const std::byte> bytes{static_cast<const std::byte*>(data), size};
        std::vector<std::byte> copy{bytes.begin(), bytes.end()};
        std::free(data);  // NOLINT(*-no-malloc, *-owning-memory): allocated by cgltf with the default allocator
        return copy;
    }

    std::string _path;                                            ///< For the messages, and to find the images.
    const cgltf_options* _options;                                ///< To decode data URIs.
    Model _model;                                                 ///< What is built.
    std::vector<bool> _knownTangents;                             ///< Per vertex: whether the file gave its tangent.
    bool _anyTangent{false};                                      ///< Whether some vertex has one from the file.
    bool _anyColor{false};                                        ///< Whether some vertex has a colour.
    bool _anyUv2{false};                                          ///< Whether some vertex has a second UV.
    std::unordered_map<const cgltf_image*, std::size_t> _images;  ///< Images already added, by their index.
};

}  // namespace

Model loadGltf(std::string_view path, const core::vfs::FileSource& source) {
    // The bytes stay alive while cgltf reads them: a .glb's binary chunk points into them.
    std::string bytes;
    try {
        bytes = source.read(path);
    } catch (const core::asset::exceptions::AssetLoadException& error) {
        throw core::asset::exceptions::AssetLoadException{std::format("Mesh '{}': {}", path, error.what())};
    }
    ReadContext context{.source = &source};
    cgltf_options options{};
    options.file.read = &readFile;
    options.file.release = &releaseFile;
    options.file.user_data = &context;

    cgltf_data* parsed = nullptr;
    if (const cgltf_result result = cgltf_parse(&options, bytes.data(), bytes.size(), &parsed);
        result != cgltf_result_success) {
        throw core::asset::exceptions::AssetLoadException{std::format("Mesh '{}': {}", path, describe(result))};
    }
    const std::unique_ptr<cgltf_data, GltfDeleter> data{parsed};
    const std::string pathText{path};
    if (const cgltf_result result = cgltf_load_buffers(&options, data.get(), pathText.c_str());
        result != cgltf_result_success) {
        throw core::asset::exceptions::AssetLoadException{std::format(
            "Mesh '{}': {}{}", path, describe(result), context.error.empty() ? "" : " (" + context.error + ")")};
    }
    if (const cgltf_result result = cgltf_validate(data.get()); result != cgltf_result_success) {
        throw core::asset::exceptions::AssetLoadException{std::format("Mesh '{}': {}", path, describe(result))};
    }

    ModelBuilder builder{path, options};
    const cgltf_scene* scene = data->scene;
    if (scene == nullptr && data->scenes_count > 0) {
        scene = data->scenes;
    }
    if (scene != nullptr) {
        for (const cgltf_node* node : std::span{scene->nodes, scene->nodes_count}) {
            builder.addNode(*node);
        }
    } else {
        for (const cgltf_mesh& mesh : std::span{data->meshes, data->meshes_count}) {
            builder.addMesh(mesh, glm::mat4{1.0F});
        }
    }
    return builder.take();
}

}  // namespace aylo::plugins::mesh::detail
