/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshTests
*/

#include <gtest/gtest.h>

#include <algorithm>
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
#include <string_view>
#include <vector>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "TestModels.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/world/World.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/vfs/DirectorySource.hpp"
#include "plugins/AssetDirectory.hpp"
#include "plugins/AssetLoaders.hpp"
#include "plugins/mesh/MeshAssets.hpp"
#include "plugins/mesh/MeshPlugin.hpp"
#include "plugins/mesh/MeshRenderer.hpp"
#include "plugins/mesh/detail/GltfLoader.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/ShaderHandles.hpp"
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

    const aylo::plugins::mesh::detail::Model model = aylo::plugins::mesh::detail::loadGltf("models/tri.gltf", source);

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

TEST(MeshPlugin, CreatesAnObjModelWithTheTextureOfItsMtl) {
    const AssetsFolder assets;
    assets.writeQuad();
    MeshApp fixture{assets};
    const aylo::core::asset::AssetId id = fixture.meshes().load("models/quad.obj");
    fixture.app().getWorld().spawn(aylo::plugins::transform::Transform3D{}, MeshRenderer{.mesh = id});

    fixture.runFrames(4);

    EXPECT_EQ(fixture.meshes().get(id)->state, MeshState::kLoaded) << fixture.meshes().get(id)->error;
    ASSERT_EQ(fixture.state().createdStaticMeshes.size(), 1U);
    EXPECT_EQ(fixture.state().createdStaticMeshes.at(0).indices.size(), 6U);
    EXPECT_NE(fixture.state().createdStaticMeshes.at(0).parts.at(0).texture, aylo::contract::renderer::TextureId{});
    EXPECT_FALSE(fixture.state().staticMeshDraws.empty());
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

namespace {
/// @brief A MeshApp whose renderer runs game shaders, with a model at the origin drawn with @p shader's material.
struct MaterialFixture {
    AssetsFolder assets;
    MeshApp fixture{assets};
    aylo::core::asset::AssetId material;

    explicit MaterialFixture(std::string_view shader) {
        assets.writeTriangle();
        fixture.state().shaders = true;
        auto& resources = fixture.app().getWorld().getResources();
        const aylo::core::asset::AssetId compiled =
            resources.getMut<aylo::plugins::shader::ShaderAssets>()->add("test.shader", shader);
        material = resources.getMut<aylo::plugins::shader::MaterialAssets>()->add(
            "test.material", aylo::plugins::shader::Material{.shader = compiled});
        const aylo::core::asset::AssetId mesh = fixture.meshes().load("models/tri.gltf");
        fixture.app().getWorld().spawn(aylo::plugins::transform::Transform3D{},
                                       MeshRenderer{.mesh = mesh, .material = material});
    }
};
}  // namespace

TEST(MeshPlugin, DrawsAModelWithItsMeshMaterial) {
    MaterialFixture test{"#pragma mesh\nuniform vec4 tint = vec4(1.0);\nvoid fragment() { COLOR *= tint; }\n"};
    test.fixture.runFrames(4);

    ASSERT_FALSE(test.fixture.state().staticMeshDraws.empty());
    EXPECT_NE(test.fixture.state().staticMeshDraws.back().material, aylo::contract::renderer::MaterialId{});
}

TEST(MeshPlugin, AModelWithA2DMaterialIsDrawnWithTheBuiltInShaderAndAWarning) {
    MaterialFixture test{"void fragment() { COLOR.r = 1.0; }\n"};
    testing::internal::CaptureStderr();
    test.fixture.runFrames(4);
    const std::string log = testing::internal::GetCapturedStderr();

    ASSERT_FALSE(test.fixture.state().staticMeshDraws.empty());
    EXPECT_EQ(test.fixture.state().staticMeshDraws.back().material, aylo::contract::renderer::MaterialId{});
    EXPECT_NE(log.find("#pragma mesh"), std::string::npos) << log;
    std::size_t warnings = 0;
    for (std::size_t at = log.find("[Mesh]"); at != std::string::npos; at = log.find("[Mesh]", at + 1)) {
        ++warnings;
    }
    EXPECT_EQ(warnings, 1U);  // once, not every frame
}

TEST(MeshPlugin, TwoModelsSharingAMaterialDrawWithTheirOwnInstanceValues) {
    MaterialFixture test{"#pragma mesh\ninstance uniform float glow = 0.5;\nvoid fragment() { COLOR.rgb *= glow; }\n"};
    auto& world = test.fixture.app().getWorld();
    const aylo::contract::renderer::CompiledShader& shader =
        *world.getResources().get<aylo::plugins::shader::ShaderAssets>()->get(
            world.getResources().get<aylo::plugins::shader::MaterialAssets>()->get(test.material)->shader);
    world.spawn(aylo::plugins::transform::Transform3D{},
                MeshRenderer{.mesh = test.fixture.meshes().load("models/tri.gltf"), .material = test.material},
                aylo::plugins::shader::MaterialInstance::of(shader).set(shader, "glow", 0.9F));
    test.fixture.runFrames(4);

    const auto& draws = test.fixture.state().staticMeshDraws;
    ASSERT_GE(draws.size(), 2U);
    const float first = draws.at(draws.size() - 2).instanceValues.at(0).x;
    const float second = draws.back().instanceValues.at(0).x;
    EXPECT_FLOAT_EQ(std::min(first, second), 0.5F);  // the shader's default, without a MaterialInstance
    EXPECT_FLOAT_EQ(std::max(first, second), 0.9F);
}

TEST(MeshPlugin, DrawsEachPartWithItsOwnMaterialAndThenTheNextPasses) {
    AssetsFolder assets;
    assets.writeTwoParts();
    MeshApp fixture{assets};
    fixture.state().shaders = true;
    auto& resources = fixture.app().getWorld().getResources();
    auto shaders = resources.getMut<aylo::plugins::shader::ShaderAssets>();
    auto materials = resources.getMut<aylo::plugins::shader::MaterialAssets>();
    const aylo::core::asset::AssetId shader = shaders->add("plain", "#pragma mesh\nvoid fragment() {}\n");
    const aylo::core::asset::AssetId outline = materials->add("outline", {.shader = shader});
    const aylo::core::asset::AssetId paint = materials->add("paint", {.shader = shader, .nextPass = outline});
    const aylo::core::asset::AssetId glass = materials->add("glass", {.shader = shader});
    fixture.app().getWorld().spawn(
        aylo::plugins::transform::Transform3D{},
        MeshRenderer{.mesh = fixture.meshes().load("models/parts.gltf"), .material = paint, .materials = {{}, glass}});
    fixture.runFrames(4);

    // The last frame: the model with paint on the body and glass on the other part, then paint's outline on the body.
    const auto& draws = fixture.state().staticMeshDraws;
    const auto& parts = fixture.state().staticMeshPartMaterials;
    ASSERT_GE(draws.size(), 2U);
    const auto* handles = &*resources.get<aylo::plugins::shader::ShaderHandles>();
    const auto created = [handles](aylo::core::asset::AssetId id) { return handles->getMaterial(id)->material; };
    const std::size_t model = draws.size() - 2;
    EXPECT_EQ(draws.at(model).material, created(paint));
    EXPECT_FALSE(draws.at(model).listedPartsOnly);
    EXPECT_EQ(parts.at(model), (std::vector<aylo::contract::renderer::MaterialId>{{}, created(glass)}));
    EXPECT_TRUE(draws.back().listedPartsOnly);
    EXPECT_EQ(parts.back(), (std::vector<aylo::contract::renderer::MaterialId>{created(outline), {}}));
}

TEST(MeshPlugin, StopsAChainOfNextPassesThatLoopsAndSaysSo) {
    AssetsFolder assets;
    assets.writeTriangle();
    MeshApp fixture{assets};
    fixture.state().shaders = true;
    auto& resources = fixture.app().getWorld().getResources();
    auto materials = resources.getMut<aylo::plugins::shader::MaterialAssets>();
    const aylo::core::asset::AssetId shader =
        resources.getMut<aylo::plugins::shader::ShaderAssets>()->add("plain", "#pragma mesh\nvoid fragment() {}\n");
    const aylo::core::asset::AssetId ping = materials->add("ping", {.shader = shader});
    const aylo::core::asset::AssetId pong = materials->add("pong", {.shader = shader, .nextPass = ping});
    materials->setNextPass(ping, pong);
    fixture.app().getWorld().spawn(aylo::plugins::transform::Transform3D{},
                                   MeshRenderer{.mesh = fixture.meshes().load("models/tri.gltf"), .material = ping});
    testing::internal::CaptureStderr();
    fixture.runFrames(4);
    const std::string log = testing::internal::GetCapturedStderr();
    EXPECT_NE(log.find("next passes chain more than 8"), std::string::npos) << log;
    EXPECT_LE(fixture.state().staticMeshDraws.size(), 4U * 8U);
}

TEST(MeshPlugin, CreatesAModelWithItsMapsReadAsData) {
    AssetsFolder assets;
    assets.writePanel();
    MeshApp fixture{assets};
    fixture.app().getWorld().spawn(aylo::plugins::transform::Transform3D{},
                                   MeshRenderer{.mesh = fixture.meshes().load("models/panel.gltf")});
    fixture.runFrames(4);

    ASSERT_EQ(fixture.state().createdStaticMeshes.size(), 1U);
    const auto& part = fixture.state().createdStaticMeshes.at(0).parts.at(0);
    EXPECT_NE(part.texture, aylo::contract::renderer::TextureId{});
    EXPECT_NE(part.normalTexture, aylo::contract::renderer::TextureId{});
    EXPECT_NE(part.metallicRoughnessTexture, aylo::contract::renderer::TextureId{});
    EXPECT_NE(part.emissiveTexture, aylo::contract::renderer::TextureId{});
    EXPECT_NE(part.normalTexture, part.texture);  // the same image, read linear for the normal map
}

TEST(MeshPlugin, TellsTheRendererWhetherAModelCastsAndReceivesShadows) {
    AssetsFolder assets;
    assets.writeTriangle();
    MeshApp fixture{assets};
    fixture.app().getWorld().spawn(
        aylo::plugins::transform::Transform3D{},
        MeshRenderer{.mesh = fixture.meshes().load("models/tri.gltf"), .castShadows = false, .receiveShadows = false});
    fixture.runFrames(4);
    ASSERT_FALSE(fixture.state().staticMeshDraws.empty());
    EXPECT_FALSE(fixture.state().staticMeshDraws.back().castShadows);
    EXPECT_FALSE(fixture.state().staticMeshDraws.back().receiveShadows);
}

TEST(MeshPlugin, LoadsAModelOfTheAssetLoaders) {
    // What a capsule names reaches the mesh plugin through the AssetLoaders, as engine.assets.mesh would (#152).
    const AssetsFolder assets;
    assets.writeTriangle();
    MeshApp mesh{assets};
    const auto* loaders = mesh.app().getWorld().getResources().get<aylo::plugins::AssetLoaders>();
    ASSERT_NE(loaders, nullptr);

    EXPECT_TRUE(loaders->load("models/tri.gltf"));

    EXPECT_NE(mesh.meshes().get(aylo::core::asset::AssetId::fromPath("models/tri.gltf")), nullptr);
}
