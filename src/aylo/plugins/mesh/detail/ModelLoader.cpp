/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ModelLoader
*/

#include "ModelLoader.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

#include "GltfLoader.hpp"
#include "Model.hpp"
#include "UfbxLoader.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/vfs/FileSource.hpp"

namespace aylo::plugins::mesh::detail {
namespace {

/// @brief The loaders, by format.
enum class Format : std::uint8_t { kUnknown, kGltf, kUfbx };

/// @return The format of @p path, by its extension in lower case.
Format formatOf(std::string_view path) {
    std::string extension = std::filesystem::path{path}.extension().string();
    std::ranges::transform(extension, extension.begin(),
                           [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    if (extension == ".gltf" || extension == ".glb") {
        return Format::kGltf;
    }
    if (extension == ".fbx" || extension == ".obj") {
        return Format::kUfbx;
    }
    return Format::kUnknown;
}

}  // namespace

bool isModelFile(std::string_view path) { return formatOf(path) != Format::kUnknown; }

Model loadModel(std::string_view path, const core::vfs::FileSource& source) {
    switch (formatOf(path)) {
        case Format::kGltf:
            return loadGltf(path, source);
        case Format::kUfbx:
            return loadUfbx(path, source);
        case Format::kUnknown:
            break;
    }
    throw core::asset::exceptions::AssetLoadException{
        std::format("Mesh '{}': its format is not read; a model is a .gltf, .glb, .fbx or .obj file", path)};
}

}  // namespace aylo::plugins::mesh::detail
