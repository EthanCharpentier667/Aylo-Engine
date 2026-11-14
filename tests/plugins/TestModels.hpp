/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TestModels
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

/// @brief A Wavefront OBJ square of side 1 in the XY plane, facing +Z, its one face a quad, with the material "Orange"
/// of quad.mtl.
inline constexpr std::string_view kQuadObj = R"(mtllib quad.mtl
v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
vt 0 0
vt 1 0
vt 1 1
vt 0 1
vn 0 0 1
usemtl Orange
f 1/1/1 2/2/1 3/3/1 4/4/1
)";

/// @brief The material of kQuadObj: an orange colour and the texture tex.png next to it.
inline constexpr std::string_view kQuadMtl = R"(newmtl Orange
Kd 1 0.5 0.25
map_Kd tex.png
)";

/// @brief An ASCII FBX 7.4 triangle in centimetres (UnitScaleFactor 1), its node 5 cm up, with a red Lambert material.
inline constexpr std::string_view kCentimetreFbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
	FBXHeaderVersion: 1003
	FBXVersion: 7400
}
GlobalSettings:  {
	Version: 1000
	Properties70:  {
		P: "UpAxis", "int", "Integer", "",1
		P: "UpAxisSign", "int", "Integer", "",1
		P: "FrontAxis", "int", "Integer", "",2
		P: "FrontAxisSign", "int", "Integer", "",1
		P: "CoordAxis", "int", "Integer", "",0
		P: "CoordAxisSign", "int", "Integer", "",1
		P: "UnitScaleFactor", "double", "Number", "",1
	}
}
Objects:  {
	Geometry: 1000, "Geometry::Tri", "Mesh" {
		Vertices: *9 {
			a: 0,0,0,100,0,0,0,100,0
		}
		PolygonVertexIndex: *3 {
			a: 0,1,-3
		}
	}
	Model: 2000, "Model::Tri", "Mesh" {
		Version: 232
		Properties70:  {
			P: "Lcl Translation", "Lcl Translation", "", "A",0,5,0
		}
	}
	Material: 3000, "Material::Red", "" {
		Version: 102
		ShadingModel: "lambert"
		Properties70:  {
			P: "DiffuseColor", "Color", "", "A",1,0,0
		}
	}
}
Connections:  {
	C: "OO",2000,0
	C: "OO",1000,2000
	C: "OO",3000,2000
}
)";

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

    /// @brief Writes the OBJ square and its files under models/: quad.obj, quad.mtl and tex.png.
    void writeQuad() const {
        write("models/quad.obj", kQuadObj);
        write("models/quad.mtl", kQuadMtl);
        write("models/tex.png", aylo::test::plugins::twoPixelPng());
    }

    [[nodiscard]] const std::filesystem::path& root() const { return _root; }

  private:
    std::filesystem::path _root;
};

}  // namespace aylo::test::plugins
