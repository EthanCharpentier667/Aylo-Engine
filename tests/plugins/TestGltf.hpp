/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TestGltf
*/

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "TestImages.hpp"

namespace aylo::test::plugins {

/// @brief The glTF of one triangle with UVs, 5 units up, a base colour and a texture next to it.
inline constexpr std::string_view kTriangle = R"({
  "asset": {"version": "2.0"}, "scene": 0, "scenes": [{"nodes": [0]}],
  "nodes": [{"mesh": 0, "translation": [0, 5, 0]}],
  "meshes": [{"primitives": [{"attributes": {"POSITION": 0, "TEXCOORD_0": 1}, "indices": 2, "material": 0}]}],
  "materials": [{"pbrMetallicRoughness": {"baseColorFactor": [1, 0.5, 0.25, 1], "baseColorTexture": {"index": 0}}}],
  "textures": [{"source": 0}], "images": [{"uri": "tex.png"}],
  "buffers": [{"uri": "tri.bin", "byteLength": 68}],
  "bufferViews": [{"buffer": 0, "byteOffset": 0, "byteLength": 36}, {"buffer": 0, "byteOffset": 36, "byteLength": 24},
                  {"buffer": 0, "byteOffset": 60, "byteLength": 6}],
  "accessors": [{"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3", "min": [0, 0, 0], "max": [1, 1, 0]},
                {"bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC2"},
                {"bufferView": 2, "componentType": 5123, "count": 3, "type": "SCALAR"}]
})";

/// @return The 68 bytes of tri.bin: three positions, three UVs, three 16-bit indices and two bytes of padding.
inline std::vector<std::byte> triangleBuffer() {
    const std::array<float, 15> floats{0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1};
    const std::array<std::uint16_t, 4> indices{0, 1, 2, 0};
    std::vector<std::byte> bytes(68);
    std::memcpy(bytes.data(), floats.data(), sizeof(floats));
    std::memcpy(bytes.data() + sizeof(floats), indices.data(), 8);  // NOLINT(*-pointer-arithmetic)
    return bytes;
}

/// @brief An assets folder in the temporary directory, removed on destruction.
class AssetsFolder {
  public:
    AssetsFolder() {
        static std::atomic<int> counter{0};
        _root = std::filesystem::temp_directory_path() / std::format("aylo_mesh_tests_{}", ++counter);
        std::filesystem::create_directories(_root / "models");
    }
    ~AssetsFolder() { std::filesystem::remove_all(_root); }
    AssetsFolder(const AssetsFolder&) = delete;
    AssetsFolder& operator=(const AssetsFolder&) = delete;
    AssetsFolder(AssetsFolder&&) = delete;
    AssetsFolder& operator=(AssetsFolder&&) = delete;

    void write(const std::string& path, std::span<const std::byte> bytes) const {
        std::ofstream{_root / path, std::ios::binary}.write(reinterpret_cast<const char*>(bytes.data()),  // NOLINT
                                                            static_cast<std::streamsize>(bytes.size()));
    }
    void write(const std::string& path, std::string_view text) const { write(path, std::as_bytes(std::span{text})); }

    /// @brief Writes the triangle and its files under models/: tri.gltf, tri.bin and tex.png.
    void writeTriangle() const {
        write("models/tri.gltf", kTriangle);
        write("models/tri.bin", triangleBuffer());
        write("models/tex.png", aylo::test::plugins::twoPixelPng());
    }

    [[nodiscard]] const std::filesystem::path& root() const { return _root; }

  private:
    std::filesystem::path _root;
};

}  // namespace aylo::test::plugins
