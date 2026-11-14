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

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
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

/// @brief Builds a GltfModel from a parsed file.
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
    GltfModel take() {
        if (_model.mesh.indices.empty()) {
            throw core::asset::exceptions::AssetLoadException{
                std::format("Mesh '{}': it has no triangles to draw", _path)};
        }
        return std::move(_model);
    }

  private:
    /// @brief The accessors of a primitive's attributes the model uses; null for one it does not have.
    struct Attributes {
        const cgltf_accessor* positions = nullptr;  ///< POSITION.
        const cgltf_accessor* normals = nullptr;    ///< NORMAL.
        const cgltf_accessor* uvs = nullptr;        ///< TEXCOORD_0.
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
        }
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

    /// @brief Gives @p part the base colour and texture of @p material, if it has them; @return its image.
    std::optional<std::size_t> applyMaterial(const cgltf_material* material, contract::renderer::StaticMeshPart& part) {
        if (material == nullptr || material->has_pbr_metallic_roughness == 0) {
            return std::nullopt;
        }
        const cgltf_pbr_metallic_roughness& pbr = material->pbr_metallic_roughness;
        part.baseColor = glm::make_vec4(static_cast<const cgltf_float*>(pbr.base_color_factor));
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
        const std::optional<std::size_t> image = applyMaterial(primitive.material, part);
        _model.mesh.parts.push_back(part);
        _model.partImages.push_back(image);
    }

    /// @return The index of @p image in the model's images, added the first time.
    std::optional<std::size_t> imageOf(const cgltf_image& image) {
        if (const auto known = _images.find(&image); known != _images.end()) {
            return known->second;
        }
        GltfImage made;
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
    GltfModel _model;                                             ///< What is built.
    std::unordered_map<const cgltf_image*, std::size_t> _images;  ///< Images already added, by their index.
};

}  // namespace

GltfModel loadGltf(std::string_view path, const core::vfs::FileSource& source) {
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
