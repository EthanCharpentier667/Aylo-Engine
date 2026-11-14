/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MaterialFileTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <glm/ext/vector_float4.hpp>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>
#include <vector>

#include "aylo/core/reload/ChangeSet.hpp"
#include "aylo/core/reload/ReloadOutcome.hpp"
#include "core/asset/AssetId.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialFile.hpp"
#include "plugins/shader/MaterialReloadable.hpp"
#include "plugins/shader/ShaderAssets.hpp"

using aylo::core::asset::AssetId;
using aylo::core::asset::exceptions::AssetLoadException;
using aylo::core::reload::ChangeSet;
using aylo::core::reload::ReloadOutcome;
using aylo::plugins::shader::Material;
using aylo::plugins::shader::MaterialFile;

namespace {
constexpr std::string_view kShader = R"(uniform vec4 tint = vec4(1.0);
uniform int mode = 0;
uniform sampler2D noise;
instance uniform float amount = 0.0;
void fragment() { COLOR *= tint * texture(noise, UV).r * float(mode) * amount; }
)";
constexpr std::string_view kMaterial = R"({
  "shader": "shaders/tint.glsl",
  "values": { "tint": [1, 0.5, 0.25, 1], "mode": 2 },
  "textures": { "noise": "textures/noise.png" }
})";

/// @brief A game's assets of their own, with a shader, removed when the test ends.
class MaterialFileTest : public ::testing::Test {
  protected:
    void SetUp() override {
        std::filesystem::create_directories(_game / "assets/shaders");
        std::filesystem::create_directories(_game / "assets/materials");
        _shaders.setDirectory(_game / "assets");
        write("shaders/tint.glsl", kShader);
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_game, error);
    }

    void write(std::string_view path, std::string_view text) const {
        std::ofstream{_game / "assets" / path, std::ios::binary} << text;
    }

    AssetId load(std::string_view path) {
        return aylo::plugins::shader::loadMaterialFile(path, _shaders, _materials, _textures);
    }

    std::filesystem::path _game{std::filesystem::temp_directory_path() /
                                std::format("aylo_material_file_{:08x}", std::random_device{}())};
    aylo::plugins::shader::ShaderAssets _shaders;
    aylo::plugins::shader::MaterialAssets _materials;
    aylo::plugins::graphics::TextureAssets _textures;
};
}  // namespace

TEST(MaterialFile, ReadsItsShaderValuesAndTextures) {
    const MaterialFile file = MaterialFile::parse(kMaterial, "materials/hit.material");
    EXPECT_EQ(file.shader, "shaders/tint.glsl");
    ASSERT_EQ(file.values.size(), 2U);
    EXPECT_EQ(file.values.at(0).name, "tint");
    EXPECT_EQ(file.values.at(0).components, (std::vector<float>{1.0F, 0.5F, 0.25F, 1.0F}));
    EXPECT_EQ(file.values.at(1).components, (std::vector<float>{2.0F}));
    ASSERT_EQ(file.textures.size(), 1U);
    EXPECT_EQ(file.textures.at(0).path, "textures/noise.png");
}

TEST(MaterialFile, WritesWhatItReads) {
    const MaterialFile file = MaterialFile::parse(kMaterial, "materials/hit.material");
    const MaterialFile again = MaterialFile::parse(file.write(), "materials/hit.material");
    EXPECT_EQ(again.shader, file.shader);
    EXPECT_EQ(again.values.at(0).components, file.values.at(0).components);
    EXPECT_EQ(again.values.at(1).components, file.values.at(1).components);
    EXPECT_EQ(again.textures.at(0).path, file.textures.at(0).path);
    EXPECT_TRUE(MaterialFile{}.write().ends_with("\n"));
}

TEST(MaterialFile, NamesTheFileAndWhatIsWrong) {
    const auto failsWith = [](std::string_view text, std::string_view expected) {
        try {
            static_cast<void>(MaterialFile::parse(text, "materials/bad.material"));
            ADD_FAILURE() << "expected an error for " << text;
        } catch (const AssetLoadException& error) {
            const std::string message = error.what();
            EXPECT_TRUE(message.contains("materials/bad.material")) << message;
            EXPECT_TRUE(message.contains(expected)) << message;
        }
    };
    failsWith("{ not json", "not a JSON object");
    failsWith(R"({ "colour": 1 })", "unknown key 'colour'");
    failsWith(R"({ "values": { "tint": [1, 2, 3, 4, 5] } })", "value 'tint'");
    failsWith(R"({ "values": { "tint": "red" } })", "value 'tint'");
    failsWith(R"({ "textures": { "noise": 3 } })", "texture 'noise'");
}

TEST_F(MaterialFileTest, LoadsTheMaterialWithItsShaderTypes) {
    write("materials/hit.material", kMaterial);
    const AssetId id = load("materials/hit.material");

    EXPECT_EQ(id, AssetId::fromPath("materials/hit.material"));
    const Material* material = _materials.get(id);
    ASSERT_NE(material, nullptr);
    EXPECT_EQ(material->shader, AssetId::fromPath("shaders/tint.glsl"));
    ASSERT_EQ(material->values.size(), 2U);
    EXPECT_EQ(std::get<glm::vec4>(material->values.at(0).value), (glm::vec4{1.0F, 0.5F, 0.25F, 1.0F}));
    EXPECT_EQ(std::get<std::int32_t>(material->values.at(1).value), 2);  // the shader declares an int
    ASSERT_EQ(material->textures.size(), 1U);
    EXPECT_NE(_textures.get(material->textures.at(0).texture), nullptr);  // queued for loading
}

TEST_F(MaterialFileTest, ReadAgainItReplacesItself) {
    write("materials/hit.material", kMaterial);
    const AssetId id = load("materials/hit.material");
    static_cast<void>(_materials.takeChanged());
    write("materials/hit.material", R"({ "shader": "shaders/tint.glsl", "values": { "mode": 5 } })");

    EXPECT_EQ(load("materials/hit.material"), id);
    EXPECT_EQ(std::get<std::int32_t>(_materials.get(id)->values.at(0).value), 5);
    EXPECT_EQ(_materials.takeChanged(), (std::vector<AssetId>{id}));
}

TEST_F(MaterialFileTest, DescribesAMaterialToSaveIt) {
    write("materials/hit.material", kMaterial);
    const AssetId id = load("materials/hit.material");
    _materials.setValue(id, "tint", glm::vec4{0.0F, 1.0F, 0.0F, 1.0F});

    const MaterialFile file = aylo::plugins::shader::describeMaterial(id, _shaders, _materials, _textures);
    EXPECT_EQ(file.shader, "shaders/tint.glsl");
    EXPECT_EQ(file.values.at(0).components, (std::vector<float>{0.0F, 1.0F, 0.0F, 1.0F}));
    EXPECT_EQ(file.values.at(1).components, (std::vector<float>{2.0F}));
    EXPECT_EQ(file.textures.at(0).path, "textures/noise.png");
}

TEST_F(MaterialFileTest, AMissingFileIsAnError) {
    EXPECT_THROW(static_cast<void>(load("materials/missing.material")), AssetLoadException);
}

TEST_F(MaterialFileTest, TheReloadableReadsAgainOnlyTheLoadedMaterials) {
    write("materials/hit.material", kMaterial);
    const AssetId id = load("materials/hit.material");
    aylo::plugins::shader::MaterialReloadable reloadable{&_shaders, &_materials, &_textures, "assets"};
    write("materials/hit.material", R"({ "shader": "shaders/tint.glsl", "values": { "mode": 7 } })");
    write("materials/other.material", R"({ "shader": "shaders/tint.glsl" })");

    const ReloadOutcome outcome = reloadable.reload(
        ChangeSet{.added = {"assets/materials/other.material"}, .modified = {"assets/materials/hit.material"}});

    EXPECT_EQ(outcome.status, ReloadOutcome::Status::kApplied);
    EXPECT_EQ(outcome.what, (std::vector<std::string>{"material materials/hit.material"}));
    EXPECT_EQ(std::get<std::int32_t>(_materials.get(id)->values.at(0).value), 7);
    EXPECT_EQ(_materials.get(AssetId::fromPath("materials/other.material")), nullptr);  // never loaded by the game

    write("materials/hit.material", "{ broken");
    EXPECT_EQ(reloadable.reload(ChangeSet{.modified = {"assets/materials/hit.material"}}).status,
              ReloadOutcome::Status::kFailed);
    EXPECT_EQ(std::get<std::int32_t>(_materials.get(id)->values.at(0).value), 7);  // the last version is kept
}
