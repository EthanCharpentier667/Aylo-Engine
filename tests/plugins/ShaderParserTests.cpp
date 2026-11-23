/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderParserTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <string_view>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "plugins/shader/detail/ShaderParser.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"

using aylo::contract::renderer::ShaderValueType;
using aylo::plugins::shader::detail::ParsedShader;
using aylo::plugins::shader::detail::parseShader;
using aylo::plugins::shader::exceptions::ShaderException;

namespace {
/// @return The float at @p offset of @p block.
float readFloat(const ParsedShader& shader, std::uint32_t offset) {
    float value = 0.0F;
    std::memcpy(&value, shader.materialDefaults.data() + offset, sizeof(value));
    return value;
}

/// @return The message of the ShaderException parsing @p source throws, or an empty string if it throws none.
std::string errorOf(std::string_view source) {
    try {
        static_cast<void>(parseShader(source, "test"));
    } catch (const ShaderException& error) {
        return error.what();
    }
    return {};
}
}  // namespace

TEST(ShaderParser, ReadsMaterialValuesWithTheirLayoutAndDefaults) {
    const ParsedShader shader = parseShader(
        "uniform float flash = 0.5;\nuniform vec4 tint = vec4(1.0, 0.5, 0.25, 1.0);\nuniform vec2 scroll;\n"
        "void fragment() {}",
        "flash");
    ASSERT_EQ(shader.materialValues.size(), 3U);
    EXPECT_EQ(shader.materialValues.at(0).value.name, "flash");
    EXPECT_EQ(shader.materialValues.at(0).value.offset, 0U);
    EXPECT_EQ(shader.materialValues.at(1).value.type, ShaderValueType::kVec4);
    EXPECT_EQ(shader.materialValues.at(1).value.offset, 16U);  // a vec4 is aligned on 16 bytes
    EXPECT_EQ(shader.materialValues.at(2).value.offset, 32U);
    EXPECT_EQ(shader.materialValues.at(2).line, 3U);
    EXPECT_EQ(shader.materialSize, 48U);  // 40 bytes, rounded up to 16
    EXPECT_FLOAT_EQ(readFloat(shader, 0), 0.5F);
    EXPECT_FLOAT_EQ(readFloat(shader, 20), 0.5F);
    EXPECT_FLOAT_EQ(readFloat(shader, 32), 0.0F);
}

TEST(ShaderParser, PacksAFloatAfterAVec3) {
    const ParsedShader shader = parseShader("uniform vec3 color;\nuniform float alpha;\nvoid fragment() {}", "x");
    EXPECT_EQ(shader.materialValues.at(1).value.offset, 12U);
    EXPECT_EQ(shader.materialSize, 16U);
}

TEST(ShaderParser, ReadsInstanceValuesIntoTheirOwnBlock) {
    const ParsedShader shader = parseShader(
        "instance uniform float flash = 1.0;\ninstance uniform vec4 tint = vec4(0.5);\nvoid fragment() {}", "x");
    ASSERT_EQ(shader.instanceValues.size(), 2U);
    EXPECT_EQ(shader.instanceValues.at(1).value.offset, 16U);
    EXPECT_TRUE(shader.materialValues.empty());
    EXPECT_EQ(shader.materialSize, 0U);
    EXPECT_FLOAT_EQ(shader.instanceDefaults.at(0).x, 1.0F);
    EXPECT_EQ(shader.instanceDefaults.at(1), glm::vec4{0.5F});
}

TEST(ShaderParser, ReadsTexturesInSlotOrder) {
    const ParsedShader shader =
        parseShader("uniform sampler2D noise;\nuniform sampler2D palette;\nvoid fragment() {}", "x");
    ASSERT_EQ(shader.textures.size(), 2U);
    EXPECT_EQ(shader.textures.at(1).name, "palette");
    EXPECT_EQ(shader.textures.at(1).slot, 1U);
}

TEST(ShaderParser, StoresAnIntDefaultAsAnInt) {
    const ParsedShader shader = parseShader("uniform int frames = 4;\nvoid fragment() {}", "x");
    std::int32_t value = 0;
    std::memcpy(&value, shader.materialDefaults.data(), sizeof(value));
    EXPECT_EQ(value, 4);
}

TEST(ShaderParser, KeepsTheLinesOfTheSourceAndBlanksTheDeclarations) {
    const ParsedShader shader = parseShader("uniform float a; // tint\n\nvoid fragment() {\n    COLOR *= a;\n}\n", "x");
    EXPECT_EQ(shader.body, "\n\nvoid fragment() {\n    COLOR *= a;\n}\n");
}

TEST(ShaderParser, FindsTheVertexAndFragmentFunctions) {
    const ParsedShader both = parseShader("void vertex () {}\nvoid\nfragment( ) {}", "x");
    EXPECT_TRUE(both.hasVertex);
    EXPECT_TRUE(both.hasFragment);
    const ParsedShader commented = parseShader("// void vertex() {}\nvoid fragment() {}", "x");
    EXPECT_FALSE(commented.hasVertex);
}

TEST(ShaderParser, IgnoresDeclarationsInBlockComments) {
    const ParsedShader shader = parseShader("/*\nuniform float a;\n*/\nvoid fragment() {}", "x");
    EXPECT_TRUE(shader.materialValues.empty());
}

TEST(ShaderParser, NamesTheShaderAndTheLineOfAnError) {
    EXPECT_EQ(errorOf("void fragment() {}\nuniform mat4 m;"),
              "Shader 'test', line 2: type 'mat4' is not float, vec2, vec3, vec4, int or sampler2D");
}

TEST(ShaderParser, RejectsWhatItCannotRead) {
    EXPECT_NE(errorOf("uniform float a\nvoid fragment() {}"), "");                    // no ';'
    EXPECT_NE(errorOf("uniform float a; uniform float b;\nvoid fragment() {}"), "");  // two on a line
    EXPECT_NE(errorOf("uniform float a[4];\nvoid fragment() {}"), "");                // an array
    EXPECT_NE(errorOf("uniform vec3 a = vec3(1, 2);\nvoid fragment() {}"), "");       // two numbers for three
    EXPECT_NE(errorOf("uniform float a = pi;\nvoid fragment() {}"), "");              // not a number
    EXPECT_NE(errorOf("instance uniform sampler2D t;\nvoid fragment() {}"), "");      // an instance texture
}

TEST(ShaderParser, RejectsReservedAndDuplicateNames) {
    EXPECT_NE(errorOf("uniform vec4 COLOR;\nvoid fragment() {}").find("reserved"), std::string::npos);
    EXPECT_NE(errorOf("uniform float ayloX;\nvoid fragment() {}").find("reserved"), std::string::npos);
    EXPECT_NE(errorOf("uniform float a;\ninstance uniform float a;\nvoid fragment() {}").find("twice"),
              std::string::npos);
}

TEST(ShaderParser, RejectsInstanceValuesPastTheBudgetAndTooManyTextures) {
    std::string instances;
    for (int i = 0; i < 5; ++i) {
        instances += "instance uniform vec4 v" + std::to_string(i) + ";\n";
    }
    EXPECT_NE(errorOf(instances + "void fragment() {}").find("64 bytes"), std::string::npos);
    EXPECT_NE(errorOf("uniform sampler2D a;\nuniform sampler2D b;\nuniform sampler2D c;\nuniform sampler2D d;\n"
                      "void fragment() {}")
                  .find("more than 3 textures"),
              std::string::npos);
}

TEST(ShaderParser, RejectsAShaderWithoutFunction) {
    EXPECT_NE(errorOf("uniform float a;").find("neither"), std::string::npos);
}

TEST(ShaderParser, ReadsTheScreenPragmaAndBlanksItsLine) {
    const ParsedShader shader = parseShader("#pragma screen\nvoid fragment() {}", "test");
    EXPECT_TRUE(shader.screen);
    EXPECT_EQ(shader.body, "\nvoid fragment() {}");
    EXPECT_FALSE(parseShader("void fragment() {}", "test").screen);
}

TEST(ShaderParser, ReservesTheScreenBuiltins) {
    for (const std::string_view name : {"SCREEN_TEXTURE", "SCREEN_UV", "SCREEN_PIXEL_SIZE"}) {
        const std::string message = errorOf(std::string{"uniform vec2 "} + std::string{name} + ";\nvoid fragment() {}");
        EXPECT_NE(message.find("reserved"), std::string::npos) << name;
    }
}

TEST(ShaderParser, ReservesTheMeshBuiltins) {
    for (const std::string_view name :
         {"NORMAL", "MODEL_MATRIX", "INV_MODEL_MATRIX", "NORMAL_MATRIX", "AABB_MIN", "AABB_MAX"}) {
        const std::string message = errorOf(std::string{"uniform vec3 "} + std::string{name} + ";\nvoid fragment() {}");
        EXPECT_NE(message.find("reserved"), std::string::npos) << name;
    }
}

TEST(ShaderParser, ReadsVaryingsAndBlanksTheirLines) {
    const ParsedShader shader =
        parseShader("varying vec3 position;\nvarying int seed;\nvoid vertex() { position = vec3(VERTEX, 0.0); }", "v");
    ASSERT_EQ(shader.varyings.size(), 2U);
    EXPECT_EQ(shader.varyings.at(0).type, "vec3");
    EXPECT_EQ(shader.varyings.at(0).name, "position");
    EXPECT_EQ(shader.varyings.at(1).type, "int");
    EXPECT_EQ(shader.varyings.at(1).line, 2U);
    EXPECT_EQ(shader.body, "\n\nvoid vertex() { position = vec3(VERTEX, 0.0); }");
}

TEST(ShaderParser, RejectsAVaryingItCannotPass) {
    EXPECT_NE(errorOf("varying mat4 m;\nvoid fragment() {}").find("line 1"), std::string::npos);  // a type it has not
    EXPECT_NE(errorOf("varying vec3 a[2];\nvoid fragment() {}"), "");                             // an array
    EXPECT_NE(errorOf("varying vec3 a\nvoid fragment() {}"), "");                                 // no ';'
    EXPECT_NE(errorOf("varying vec3 UV;\nvoid fragment() {}").find("reserved"), std::string::npos);
    EXPECT_NE(errorOf("uniform float a;\nvarying float a;\nvoid fragment() {}").find("twice"), std::string::npos);
    std::string many;
    for (int index = 0; index <= 5; ++index) {
        many += "varying float v" + std::to_string(index) + ";\n";
    }
    EXPECT_NE(errorOf(many + "void fragment() {}").find("more than 5 varyings"), std::string::npos);
}

TEST(ShaderParser, ReservesThe3DViewBuiltins) {
    for (const std::string_view name :
         {"WORLD_POSITION", "LOCAL_POSITION", "VIEW_DIR", "CAMERA_POSITION", "VIEW_MATRIX", "PROJECTION_MATRIX",
          "INV_VIEW_MATRIX", "INV_PROJECTION_MATRIX", "FRONT_FACING", "FRAGCOORD"}) {
        const std::string message = errorOf(std::string{"uniform vec3 "} + std::string{name} + ";\nvoid fragment() {}");
        EXPECT_NE(message.find("reserved"), std::string::npos) << name;
    }
}

TEST(ShaderParser, ReadsTheRenderStatePragmasOfA3DShader) {
    using aylo::contract::renderer::CullMode;
    using aylo::contract::renderer::DepthMode;
    using aylo::contract::renderer::MeshBlend;
    const ParsedShader hull = parseShader(
        "#pragma mesh\n#pragma cull front\n#pragma depth_bias 2 1.5\n#pragma depth notest\nvoid fragment() {}", "hull");
    EXPECT_EQ(hull.renderState.cull, CullMode::kFront);
    EXPECT_EQ(hull.renderState.depth, DepthMode::kNoTest);
    EXPECT_EQ(hull.renderState.depthBias, 2.0F);
    EXPECT_EQ(hull.renderState.depthBiasSlope, 1.5F);
    EXPECT_EQ(hull.body, "\n\n\n\nvoid fragment() {}");

    const ParsedShader plain = parseShader("#pragma mesh\nvoid fragment() {}", "plain");
    EXPECT_EQ(plain.renderState, aylo::contract::renderer::RenderState{});  // back faces culled, opaque, depth written

    // A blended shader that says nothing of depth tests it without writing it.
    const ParsedShader glass = parseShader("#pragma mesh\n#pragma blend alpha\nvoid fragment() {}", "glass");
    EXPECT_EQ(glass.renderState.blend, MeshBlend::kAlpha);
    EXPECT_EQ(glass.renderState.depth, DepthMode::kTestNoWrite);
}

TEST(ShaderParser, NamesAWrongRenderStatePragmaAndItsLine) {
    EXPECT_TRUE(errorOf("#pragma mesh\n#pragma cull sideways\nvoid fragment() {}").contains("line 2"));
    EXPECT_TRUE(errorOf("#pragma mesh\n#pragma blend glow\nvoid fragment() {}").contains("'additive'"));
    EXPECT_TRUE(errorOf("#pragma mesh\n#pragma depth maybe\nvoid fragment() {}").contains("'notest'"));
    EXPECT_TRUE(errorOf("#pragma mesh\n#pragma depth_bias much\nvoid fragment() {}").contains("depth_bias"));
    EXPECT_TRUE(errorOf("#pragma cull front\nvoid fragment() {}").contains("for a '#pragma mesh' shader"));
}
