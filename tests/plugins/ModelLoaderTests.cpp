/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ModelLoaderTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "TestModels.hpp"
#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/vfs/DirectorySource.hpp"
#include "plugins/mesh/detail/Model.hpp"
#include "plugins/mesh/detail/ModelLoader.hpp"
#include "plugins/mesh/detail/UfbxLoader.hpp"

namespace {
using aylo::core::asset::exceptions::AssetLoadException;
using aylo::plugins::mesh::detail::Model;
using aylo::test::plugins::AssetsFolder;

/// @return The vertex of @p model at @p x, @p y (z 0), or the end.
auto vertexAt(const Model& model, float x, float y) {
    return std::ranges::find_if(model.mesh.vertices, [x, y](const aylo::contract::renderer::StaticMeshVertex& vertex) {
        return std::abs(vertex.position.x - x) < 1e-5F && std::abs(vertex.position.y - y) < 1e-5F;
    });
}
}  // namespace

TEST(UfbxLoader, ReadsAnObjQuadAsTwoTrianglesWithItsMtlAndImage) {
    const AssetsFolder assets;
    assets.writeQuad();
    const aylo::core::vfs::DirectorySource source{assets.root()};

    const Model model = aylo::plugins::mesh::detail::loadUfbx("models/quad.obj", source);

    EXPECT_EQ(model.mesh.vertices.size(), 4U);  // the corners the two triangles share are one vertex
    EXPECT_EQ(model.mesh.indices.size(), 6U);
    const auto corner = vertexAt(model, 0.0F, 0.0F);
    ASSERT_NE(corner, model.mesh.vertices.end());
    EXPECT_FLOAT_EQ(corner->uv.y, 1.0F);  // OBJ's V starts at the bottom; the contract's at the top
    EXPECT_FLOAT_EQ(corner->normal.z, 1.0F);
    ASSERT_EQ(model.mesh.parts.size(), 1U);
    EXPECT_EQ(model.mesh.parts.at(0).indexCount, 6U);
    EXPECT_FLOAT_EQ(model.mesh.parts.at(0).baseColor.y, 0.5F);
    EXPECT_FLOAT_EQ(model.mesh.parts.at(0).baseColor.z, 0.25F);
    ASSERT_EQ(model.images.size(), 1U);
    EXPECT_EQ(model.images.at(0).path, "models/tex.png");
    ASSERT_TRUE(model.partImages.at(0));
}

TEST(UfbxLoader, AnObjWithoutItsMtlLoadsWithoutColour) {
    const AssetsFolder assets;
    assets.write("models/quad.obj", aylo::test::plugins::kQuadObj);
    const aylo::core::vfs::DirectorySource source{assets.root()};

    const Model model = aylo::plugins::mesh::detail::loadUfbx("models/quad.obj", source);

    EXPECT_EQ(model.mesh.indices.size(), 6U);
    ASSERT_EQ(model.mesh.parts.size(), 1U);
    EXPECT_FLOAT_EQ(model.mesh.parts.at(0).baseColor.x, 1.0F);
    EXPECT_TRUE(model.images.empty());
}

TEST(UfbxLoader, ConvertsAnFbxInCentimetresToMetres) {
    const AssetsFolder assets;
    assets.write("models/tri.fbx", aylo::test::plugins::kCentimetreFbx);
    const aylo::core::vfs::DirectorySource source{assets.root()};

    const Model model = aylo::plugins::mesh::detail::loadUfbx("models/tri.fbx", source);

    ASSERT_EQ(model.mesh.indices.size(), 3U);
    // 100 cm along X, its node 5 cm up: 1 m and 0.05 m.
    EXPECT_NE(vertexAt(model, 1.0F, 0.05F), model.mesh.vertices.end());
    EXPECT_NE(vertexAt(model, 0.0F, 1.05F), model.mesh.vertices.end());
    ASSERT_EQ(model.mesh.parts.size(), 1U);
    EXPECT_FLOAT_EQ(model.mesh.parts.at(0).baseColor.x, 1.0F);
    EXPECT_FLOAT_EQ(model.mesh.parts.at(0).baseColor.y, 0.0F);
}

TEST(UfbxLoader, NamesTheFileAndWhyItFails) {
    const AssetsFolder assets;
    assets.write("models/broken.fbx", std::string_view{"Kaydara FBX Binary  \0garbage", 28});
    const aylo::core::vfs::DirectorySource source{assets.root()};

    try {
        static_cast<void>(aylo::plugins::mesh::detail::loadUfbx("models/broken.fbx", source));
        FAIL() << "expected an AssetLoadException";
    } catch (const AssetLoadException& error) {
        EXPECT_NE(std::string{error.what()}.find("models/broken.fbx"), std::string::npos) << error.what();
    }
    EXPECT_THROW(static_cast<void>(aylo::plugins::mesh::detail::loadUfbx("models/missing.obj", source)),
                 AssetLoadException);
}

TEST(ModelLoader, PicksTheLoaderByExtension) {
    const AssetsFolder assets;
    assets.writeTriangle();
    assets.writeQuad();
    assets.write("models/QUAD.OBJ", aylo::test::plugins::kQuadObj);
    const aylo::core::vfs::DirectorySource source{assets.root()};

    EXPECT_EQ(aylo::plugins::mesh::detail::loadModel("models/tri.gltf", source).mesh.indices.size(), 3U);
    EXPECT_EQ(aylo::plugins::mesh::detail::loadModel("models/quad.obj", source).mesh.indices.size(), 6U);
    EXPECT_EQ(aylo::plugins::mesh::detail::loadModel("models/QUAD.OBJ", source).mesh.indices.size(), 6U);
    EXPECT_TRUE(aylo::plugins::mesh::detail::isModelFile("models/Crate.FBX"));
    EXPECT_FALSE(aylo::plugins::mesh::detail::isModelFile("models/crate.stl"));
}

TEST(ModelLoader, AFormatItDoesNotReadNamesTheFormatsItReads) {
    const AssetsFolder assets;
    assets.write("models/crate.stl", std::string_view{"solid crate"});
    const aylo::core::vfs::DirectorySource source{assets.root()};

    try {
        static_cast<void>(aylo::plugins::mesh::detail::loadModel("models/crate.stl", source));
        FAIL() << "expected an AssetLoadException";
    } catch (const AssetLoadException& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("models/crate.stl"), std::string::npos) << message;
        EXPECT_NE(message.find(".fbx"), std::string::npos) << message;
    }
}

TEST(ModelLoader, KeepsTheNameOfEachPartsMaterial) {
    aylo::test::plugins::AssetsFolder assets;
    assets.writeTwoParts();
    const aylo::core::vfs::DirectorySource source{assets.root()};
    const aylo::plugins::mesh::detail::Model model =
        aylo::plugins::mesh::detail::loadModel("models/parts.gltf", source);
    ASSERT_EQ(model.mesh.parts.size(), 2U);
    EXPECT_EQ(model.partNames, (std::vector<std::string>{"Body", "Glass"}));
}

TEST(ModelLoader, ReadsAGltfMaterialWithEveryMapAndTheVertexData) {
    using aylo::contract::renderer::AlphaMode;
    using aylo::plugins::mesh::detail::PartMap;
    aylo::test::plugins::AssetsFolder assets;
    assets.writePanel();
    const aylo::core::vfs::DirectorySource source{assets.root()};
    const aylo::plugins::mesh::detail::Model model =
        aylo::plugins::mesh::detail::loadModel("models/panel.gltf", source);
    ASSERT_EQ(model.mesh.parts.size(), 2U);
    const aylo::contract::renderer::StaticMeshPart& panel = model.mesh.parts.at(0);
    EXPECT_EQ(panel.alphaMode, AlphaMode::kMask);
    EXPECT_FLOAT_EQ(panel.alphaCutoff, 0.4F);
    EXPECT_FALSE(panel.doubleSided);
    EXPECT_FLOAT_EQ(panel.metallic, 0.5F);
    EXPECT_FLOAT_EQ(panel.roughness, 1.0F);  // glTF's default factor
    EXPECT_FLOAT_EQ(panel.normalScale, 0.8F);
    EXPECT_FLOAT_EQ(panel.occlusionStrength, 0.7F);
    EXPECT_FLOAT_EQ(panel.emissive.x, 2.0F);  // the factor times the emissive strength
    EXPECT_FLOAT_EQ(panel.uvTransform.z, 2.0F);
    EXPECT_FLOAT_EQ(panel.uvRotation, 0.3F);
    ASSERT_EQ(model.partMaps.size(), 2U);
    for (const PartMap map : {PartMap::kMetallicRoughness, PartMap::kNormal, PartMap::kOcclusion, PartMap::kEmissive}) {
        EXPECT_TRUE(model.partMaps.at(0).at(static_cast<std::size_t>(map)).has_value());
        EXPECT_FALSE(model.partMaps.at(1).at(static_cast<std::size_t>(map)).has_value());
    }
    const aylo::contract::renderer::StaticMeshPart& glass = model.mesh.parts.at(1);
    EXPECT_EQ(glass.alphaMode, AlphaMode::kBlend);
    EXPECT_TRUE(glass.doubleSided);
    // The colours and second UVs of the first primitive, defaults for the second; tangents made for the normal map.
    ASSERT_EQ(model.mesh.colors.size(), model.mesh.vertices.size());
    EXPECT_FLOAT_EQ(model.mesh.colors.at(0).y, 0.0F);
    EXPECT_FLOAT_EQ(model.mesh.colors.back().y, 1.0F);
    EXPECT_EQ(model.mesh.uv2.size(), model.mesh.vertices.size());
    ASSERT_EQ(model.mesh.tangents.size(), model.mesh.vertices.size());
    EXPECT_NEAR(model.mesh.tangents.at(0).x, 1.0F, 1e-4F);  // U runs along +X
}

TEST(ModelLoader, AModelWithoutTheNewDataKeepsNone) {
    aylo::test::plugins::AssetsFolder assets;
    assets.writeTriangle();
    const aylo::core::vfs::DirectorySource source{assets.root()};
    const aylo::plugins::mesh::detail::Model model = aylo::plugins::mesh::detail::loadModel("models/tri.gltf", source);
    EXPECT_TRUE(model.mesh.tangents.empty());
    EXPECT_TRUE(model.mesh.colors.empty());
    EXPECT_TRUE(model.mesh.uv2.empty());
}
