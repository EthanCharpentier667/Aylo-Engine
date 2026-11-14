/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MaterialFile
*/

#include "plugins/shader/MaterialFile.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "core/asset/AssetId.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/ShaderAssets.hpp"

namespace aylo::plugins::shader {
namespace {
using core::asset::exceptions::AssetLoadException;

/// @throws AssetLoadException Naming @p path and saying @p what.
[[noreturn]] void fail(std::string_view path, std::string_view what) {
    throw AssetLoadException{std::format("Material '{}': {}", path, what)};
}

/// @return The numbers of the value @p name: one number, or an array of 2 to 4.
std::vector<float> readComponents(const nlohmann::ordered_json& value, std::string_view name, std::string_view path) {
    if (value.is_number()) {
        return {value.get<float>()};
    }
    if (!value.is_array() || value.size() < 2 || value.size() > 4) {
        fail(path, std::format("value '{}' must be a number or 2 to 4 numbers", name));
    }
    std::vector<float> components;
    for (const nlohmann::ordered_json& component : value) {
        if (!component.is_number()) {
            fail(path, std::format("value '{}' must be a number or 2 to 4 numbers", name));
        }
        components.push_back(component.get<float>());
    }
    return components;
}

/// @return The value of @p components, an `int` when the shader declares @p name as one and it is whole.
MaterialValue toValue(const std::vector<float>& components, bool integer) {
    switch (components.size()) {
        case 1:
            if (integer && std::trunc(components.front()) == components.front()) {
                return static_cast<std::int32_t>(components.front());
            }
            return components.front();
        case 2:
            return glm::vec2{components.at(0), components.at(1)};
        case 3:
            return glm::vec3{components.at(0), components.at(1), components.at(2)};
        default:
            return glm::vec4{components.at(0), components.at(1), components.at(2), components.at(3)};
    }
}

/// @return The numbers of @p value.
std::vector<float> componentsOf(const MaterialValue& value) {
    return std::visit(
        [](const auto& held) -> std::vector<float> {
            using Held = std::decay_t<decltype(held)>;
            if constexpr (std::is_same_v<Held, float>) {
                return {held};
            } else if constexpr (std::is_same_v<Held, std::int32_t>) {
                return {static_cast<float>(held)};
            } else if constexpr (std::is_same_v<Held, glm::vec2>) {
                return {held.x, held.y};
            } else if constexpr (std::is_same_v<Held, glm::vec3>) {
                return {held.x, held.y, held.z};
            } else {
                return {held.x, held.y, held.z, held.w};
            }
        },
        value);
}
}  // namespace

namespace {
/// @return The values of the object @p values.
std::vector<MaterialFile::Value> readValues(const nlohmann::ordered_json& values, std::string_view path) {
    if (!values.is_object()) {
        fail(path, "'values' must be an object of values by name");
    }
    std::vector<MaterialFile::Value> read;
    for (const auto& [name, components] : values.items()) {
        read.push_back(MaterialFile::Value{.name = name, .components = readComponents(components, name, path)});
    }
    return read;
}

/// @return The textures of the object @p textures.
std::vector<MaterialFile::Texture> readTextures(const nlohmann::ordered_json& textures, std::string_view path) {
    if (!textures.is_object()) {
        fail(path, "'textures' must be an object of image paths by name");
    }
    std::vector<MaterialFile::Texture> read;
    for (const auto& [name, image] : textures.items()) {
        if (!image.is_string()) {
            fail(path, std::format("texture '{}' must be an image path", name));
        }
        read.push_back(MaterialFile::Texture{.name = name, .path = image.get<std::string>()});
    }
    return read;
}
}  // namespace

MaterialFile MaterialFile::parse(std::string_view text, std::string_view path) {
    const nlohmann::ordered_json root = nlohmann::ordered_json::parse(text, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        fail(path, "not a JSON object");
    }
    MaterialFile file;
    for (const auto& [key, value] : root.items()) {
        if (key == "shader") {
            if (!value.is_string()) {
                fail(path, "'shader' must be a path");
            }
            file.shader = value.get<std::string>();
        } else if (key == "values") {
            file.values = readValues(value, path);
        } else if (key == "textures") {
            file.textures = readTextures(value, path);
        } else {
            fail(path, std::format("unknown key '{}' (shader, values, textures)", key));
        }
    }
    return file;
}

std::string MaterialFile::write() const {
    nlohmann::ordered_json written = nlohmann::ordered_json::object();
    for (const Value& value : values) {
        if (value.components.size() == 1) {
            written.emplace(value.name, value.components.front());
        } else {
            written.emplace(value.name, value.components);
        }
    }
    nlohmann::ordered_json images = nlohmann::ordered_json::object();
    for (const Texture& texture : textures) {
        images.emplace(texture.name, texture.path);
    }
    nlohmann::ordered_json root = nlohmann::ordered_json::object();
    root.emplace("shader", shader);
    root.emplace("values", std::move(written));
    root.emplace("textures", std::move(images));
    return root.dump(2) + "\n";
}

core::asset::AssetId loadMaterialFile(const std::filesystem::path& path, ShaderAssets& shaders,
                                      MaterialAssets& materials, graphics::TextureAssets& textures) {
    const std::string name = path.generic_string();
    std::string text;
    try {
        text = shaders.getSource().read(name);
    } catch (const AssetLoadException& error) {
        fail(name, error.what());
    }
    const MaterialFile file = MaterialFile::parse(text, name);
    Material material;
    if (!file.shader.empty()) {
        material.shader = shaders.load(file.shader);
    }
    const contract::renderer::CompiledShader* compiled = shaders.get(material.shader);
    for (const MaterialFile::Value& value : file.values) {
        bool integer = false;
        if (compiled != nullptr) {
            for (const contract::renderer::ShaderValue& declared : compiled->materialValues) {
                integer = integer ||
                          (declared.name == value.name && declared.type == contract::renderer::ShaderValueType::kInt);
            }
        }
        material.set(value.name, toValue(value.components, integer));
    }
    for (const MaterialFile::Texture& texture : file.textures) {
        material.setTexture(texture.name, textures.load(texture.path));
    }
    return materials.define(name, std::move(material));
}

MaterialFile describeMaterial(core::asset::AssetId id, const ShaderAssets& shaders, const MaterialAssets& materials,
                              const graphics::TextureAssets& textures) {
    const Material* material = materials.get(id);
    if (material == nullptr) {
        throw AssetLoadException{std::format("No material has the id {}", id.value)};
    }
    MaterialFile file;
    if (const ShaderAssets::Entry* shader = shaders.getEntry(material->shader)) {
        file.shader = shader->path;
    }
    for (const Material::Value& value : material->values) {
        file.values.push_back(MaterialFile::Value{.name = value.name, .components = componentsOf(value.value)});
    }
    for (const Material::Texture& texture : material->textures) {
        if (const graphics::TextureAssets::Entry* image = textures.get(texture.texture)) {
            file.textures.push_back(MaterialFile::Texture{.name = texture.name, .path = image->path});
        }
    }
    return file;
}

}  // namespace aylo::plugins::shader
