/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshTests
*/

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "FakeBackend.hpp"
#include "TestGltf.hpp"
#include "TestImages.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/world/World.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/vfs/DirectorySource.hpp"
#include "plugins/AssetDirectory.hpp"
#include "plugins/mesh/MeshAssets.hpp"
#include "plugins/mesh/MeshPlugin.hpp"
#include "plugins/mesh/MeshRenderer.hpp"
#include "plugins/mesh/detail/GltfLoader.hpp"
#include "plugins/transform/Transform.hpp"

namespace {
using aylo::plugins::mesh::MeshAssets;
using aylo::plugins::mesh::MeshRenderer;
using aylo::plugins::mesh::MeshState;
using aylo::test::plugins::AssetsFolder;
using aylo::test::plugins::kTriangle;

/// @brief A client App with the drawing plugins and the MeshPlugin over the fake backend, on an assets folder.
class MeshApp {
  public:
    explicit MeshApp(const AssetsFolder& assets)
        : _state{std::make_shared<aylo::test::plugins::FakeBackendState>()}, _app{aylo::ecs::RunMode::kClient} {
        _app.insertResource(aylo::plugins::AssetDirectory{.path = assets.root()});
        aylo::test::plugins::addDrawingPlugins(_app, aylo::test::plugins::makeFakeRegistry(_state),
                                               aylo::test::plugins::makeFakeSettings());
        _app.addPlugin(aylo::plugins::mesh::MeshPlugin{});
    }

    [[nodiscard]] aylo::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] aylo::test::plugins::FakeBackendState& state() noexcept { return *_state; }
    [[nodiscard]] MeshAssets& meshes() { return *_app.getWorld().getResources().getMut<MeshAssets>(); }

    void runFrames(int count) {
        for (int frame = 0; frame < count; ++frame) {
            _app.runFrame();
        }
    }

  private:
    std::shared_ptr<aylo::test::plugins::FakeBackendState> _state;
    aylo::ecs::App _app;
};
}  // namespace

TEST(GltfLoader, ReadsTheTrianglesTheirMaterialAndTheirImage) {
    const AssetsFolder assets;
    assets.writeTriangle();
    const aylo::core::vfs::DirectorySource source{assets.root()};

    const aylo::plugins::mesh::detail::GltfModel model =
        aylo::plugins::mesh::detail::loadGltf("models/tri.gltf", source);

    ASSERT_EQ(model.mesh.vertices.size(), 3U);
    EXPECT_FLOAT_EQ(model.mesh.vertices.at(1).position.x, 1.0F);
    EXPECT_FLOAT_EQ(model.mesh.vertices.at(1).position.y, 5.0F);  // the node's translation is baked in
    EXPECT_FLOAT_EQ(model.mesh.vertices.at(2).uv.y, 1.0F);
    EXPECT_EQ(model.mesh.indices, (std::vector<std::uint32_t>{0, 1, 2}));
    ASSERT_EQ(model.mesh.parts.size(), 1U);
    EXPECT_FLOAT_EQ(model.mesh.parts.at(0).baseColor.y, 0.5F);
    ASSERT_EQ(model.images.size(), 1U);
    EXPECT_EQ(model.images.at(0).path, "models/tex.png");
    ASSERT_TRUE(model.partImages.at(0));
}

TEST(GltfLoader, NamesTheFileAndWhyItFails) {
    const AssetsFolder assets;
    assets.write("models/broken.gltf", std::string_view{"{ not json"});
    assets.write("models/nobin.gltf", kTriangle);
    const aylo::core::vfs::DirectorySource source{assets.root()};

    try {
        static_cast<void>(aylo::plugins::mesh::detail::loadGltf("models/broken.gltf", source));
        FAIL() << "expected an AssetLoadException";
    } catch (const aylo::core::asset::exceptions::AssetLoadException& error) {
        EXPECT_NE(std::string{error.what()}.find("models/broken.gltf"), std::string::npos) << error.what();
    }
    EXPECT_THROW(static_cast<void>(aylo::plugins::mesh::detail::loadGltf("models/nobin.gltf", source)),
                 aylo::core::asset::exceptions::AssetLoadException);
    EXPECT_THROW(static_cast<void>(aylo::plugins::mesh::detail::loadGltf("models/missing.glb", source)),
                 aylo::core::asset::exceptions::AssetLoadException);
}

TEST(MeshPlugin, CreatesTheMeshWithItsTextureAndDrawsItsRenderers) {
    const AssetsFolder assets;
    assets.writeTriangle();
    MeshApp fixture{assets};
    const aylo::core::asset::AssetId id = fixture.meshes().load("models/tri.gltf");
    fixture.app().getWorld().spawn(aylo::plugins::transform::Transform3D{.position = {2.0F, 0.0F, 0.0F}},
                                   MeshRenderer{.mesh = id, .color = {1, 1, 1, 0.5F}});

    fixture.runFrames(4);

    EXPECT_EQ(fixture.meshes().get(id)->state, MeshState::kLoaded) << fixture.meshes().get(id)->error;
    ASSERT_EQ(fixture.state().createdStaticMeshes.size(), 1U);
    const aylo::contract::renderer::StaticMeshPart& part = fixture.state().createdStaticMeshes.at(0).parts.at(0);
    EXPECT_NE(part.texture, aylo::contract::renderer::TextureId{});  // the PNG next to it, loaded first
    ASSERT_FALSE(fixture.state().staticMeshDraws.empty());
    EXPECT_FLOAT_EQ(fixture.state().staticMeshDraws.back().world[3][0], 2.0F);
    EXPECT_FLOAT_EQ(fixture.state().staticMeshDraws.back().color.w, 0.5F);
}

TEST(MeshPlugin, AFileThatCannotBeReadFailsWithItsReason) {
    const AssetsFolder assets;
    MeshApp fixture{assets};
    const aylo::core::asset::AssetId id = fixture.meshes().load("models/missing.glb");

    testing::internal::CaptureStderr();
    fixture.runFrames(2);
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_EQ(fixture.meshes().get(id)->state, MeshState::kFailed);
    EXPECT_NE(log.find("models/missing.glb"), std::string::npos) << log;
    EXPECT_TRUE(fixture.state().createdStaticMeshes.empty());
}

TEST(MeshPlugin, ARendererWithout3DFailsTheMesh) {
    const AssetsFolder assets;
    assets.writeTriangle();
    MeshApp fixture{assets};
    fixture.state().draws3D = false;
    const aylo::core::asset::AssetId id = fixture.meshes().load("models/tri.gltf");

    testing::internal::CaptureStderr();
    fixture.runFrames(4);
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_EQ(fixture.meshes().get(id)->state, MeshState::kFailed);
    EXPECT_NE(log.find("does not draw 3D"), std::string::npos) << log;
}

TEST(MeshAssets, GivesTheSameIdForTheSamePath) {
    MeshAssets meshes;
    EXPECT_EQ(meshes.load("models/a.glb"), meshes.load("models/./a.glb"));
    EXPECT_FALSE(meshes.getMesh(meshes.load("models/a.glb")));
}
