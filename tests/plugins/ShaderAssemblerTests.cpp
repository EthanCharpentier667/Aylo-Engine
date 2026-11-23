/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderAssemblerTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "plugins/shader/detail/ShaderAssembler.hpp"
#include "plugins/shader/detail/ShaderParser.hpp"

using aylo::contract::renderer::ShaderTarget;
using aylo::plugins::shader::detail::assembleShader;
using aylo::plugins::shader::detail::blankFunction;
using aylo::plugins::shader::detail::ParsedShader;
using aylo::plugins::shader::detail::parseShader;
using aylo::plugins::shader::detail::ShaderStage;

namespace {
/// @brief A shader with material values, instance values, a texture, a helper and both functions.
constexpr std::string_view kShader =
    "uniform vec4 tint = vec4(1.0);\n"
    "instance uniform float flash;\n"
    "instance uniform int seed;\n"
    "uniform sampler2D noise;\n"
    "float wave(float x) { return sin(x + TIME); }\n"
    "void vertex() { VERTEX.y += wave(VERTEX.x); }\n"
    "void fragment() { if (texture(noise, UV).r < 0.1) { discard; } COLOR.rgb = mix(COLOR.rgb, tint.rgb, flash); }\n";

/// @return The GLSL of @p stage of kShader for @p target.
std::string assemble(ShaderTarget target, ShaderStage stage, std::string_view source = kShader) {
    return assembleShader(parseShader(source, "test"), target, stage);
}

/// @return Whether @p text contains @p part.
bool has(std::string_view text, std::string_view part) { return text.contains(part); }
}  // namespace

TEST(ShaderAssembler, BlanksOnlyTheNamedFunctionAndKeepsItsLines) {
    const std::string body = "void fragmentHelper() {}\nvoid fragment() {\n    discard;\n}\nvoid vertex() {}\n";
    const std::string blanked = blankFunction(body, "fragment");
    EXPECT_TRUE(has(blanked, "void fragmentHelper() {}"));
    EXPECT_FALSE(has(blanked, "discard"));
    EXPECT_TRUE(has(blanked, "void vertex() {}"));
    EXPECT_EQ(std::ranges::count(blanked, '\n'), std::ranges::count(body, '\n'));
    EXPECT_EQ(blankFunction(body, "missing"), body);
}

TEST(ShaderAssembler, StartsWithThePreludeThenTheGameLinesFromOne) {
    const std::string glsl = assemble(ShaderTarget::kSprite, ShaderStage::kFragment);
    // #version comes before any code (only comments may precede it).
    ASSERT_NE(glsl.find("#version 450\n"), std::string::npos);
    EXPECT_LT(glsl.find("#version 450\n"), glsl.find("layout("));
    EXPECT_FALSE(glsl.contains("#pragma aylo_"));  // every marker was replaced
    const std::size_t line = glsl.find("\n#line 1\n");
    ASSERT_NE(line, std::string::npos);
    EXPECT_LT(glsl.find("uniform sampler2D TEXTURE"), line);
    EXPECT_GT(glsl.find("float wave(float x)"), line);
}

TEST(ShaderAssembler, DeclaresTheMaterialAsAnAnonymousBlockAndItsTextures) {
    const std::string glsl = assemble(ShaderTarget::kSprite, ShaderStage::kFragment);
    EXPECT_TRUE(has(glsl,
                    "layout(set = 2, binding = 0, std140) uniform AyloMaterial {\n"
                    "    layout(offset = 0) vec4 tint;\n};"));
    EXPECT_TRUE(has(glsl, "layout(set = 2, binding = 1) uniform sampler2D noise;"));
    EXPECT_FALSE(has(glsl, "#define tint"));  // no macro: it would replace `.tint` members and swizzles too
}

TEST(ShaderAssembler, FillsTheInstanceValuesFromTheirSourceBeforeTheShader) {
    const std::string sprite = assemble(ShaderTarget::kSprite, ShaderStage::kFragment);
    EXPECT_TRUE(has(sprite, "float flash;\n"));
    EXPECT_TRUE(has(sprite,
                    "    flash = ayloInstance[0].x;\n    seed = floatBitsToInt(ayloInstance[0].y);\n"
                    "    fragment();\n"));
    const std::string particle = assemble(ShaderTarget::kParticle, ShaderStage::kVertex);
    EXPECT_TRUE(has(particle, "    flash = ayloParticles.instance[0].x;\n"));
    EXPECT_TRUE(has(particle, "vec4 instance[4];"));
    EXPECT_TRUE(has(assemble(ShaderTarget::kShape, ShaderStage::kFragment), "    flash = ayloInstance[0].x;\n"));
}

TEST(ShaderAssembler, EachStageKeepsOnlyItsOwnFunction) {
    const std::string vertex = assemble(ShaderTarget::kSprite, ShaderStage::kVertex);
    EXPECT_FALSE(has(vertex, "discard"));
    EXPECT_TRUE(has(vertex, "    vertex();\n"));
    EXPECT_TRUE(has(vertex, "float wave(float x)"));  // helpers stay in both stages
    const std::string fragment = assemble(ShaderTarget::kSprite, ShaderStage::kFragment);
    EXPECT_FALSE(has(fragment, "VERTEX.y += wave"));
    EXPECT_TRUE(has(fragment, "    fragment();\n"));
}

TEST(ShaderAssembler, CallsOnlyTheFunctionsTheShaderDefines) {
    const std::string vertex = assemble(ShaderTarget::kSprite, ShaderStage::kVertex, "void fragment() {}\n");
    EXPECT_FALSE(has(vertex, "    vertex();\n"));
}

TEST(ShaderAssembler, StartsEachTargetFromItsDefaultColour) {
    EXPECT_TRUE(
        has(assemble(ShaderTarget::kSprite, ShaderStage::kFragment), "COLOR = texture(TEXTURE, UV) * ayloColorIn;"));
    EXPECT_TRUE(has(assemble(ShaderTarget::kGlyph, ShaderStage::kFragment),
                    "COLOR = ayloColorIn * ayloCoverage(ayloUvWidth);"));
    EXPECT_TRUE(
        has(assemble(ShaderTarget::kParticle, ShaderStage::kFragment), "COLOR = texture(TEXTURE, UV) * ayloColorIn;"));
    EXPECT_TRUE(has(assemble(ShaderTarget::kShape, ShaderStage::kFragment),
                    "COLOR = (ayloColorIn * ayloFill) + (ayloOutlineColorIn * (ayloShape - ayloFill));"));
}

TEST(ShaderAssembler, ReadsEachTargetsOwnVertexInputs) {
    EXPECT_TRUE(
        has(assemble(ShaderTarget::kSprite, ShaderStage::kVertex), "layout(location = 7) in vec4 ayloInstance[4];"));
    const std::string particle = assemble(ShaderTarget::kParticle, ShaderStage::kVertex);
    EXPECT_TRUE(has(particle, "layout(location = 5) in float ayloLife;"));
    EXPECT_FALSE(has(particle, "ayloInstance"));
    const std::string shape = assemble(ShaderTarget::kShape, ShaderStage::kVertex);
    EXPECT_TRUE(has(shape, "layout(location = 7) in vec4 ayloOutlineColor;"));
    EXPECT_TRUE(has(shape, "layout(location = 8) in vec4 ayloInstance[4];"));
}

TEST(ShaderAssembler, CarriesTheInstanceValuesOnlyWhenTheShaderHasSome) {
    EXPECT_TRUE(assemble(ShaderTarget::kSprite, ShaderStage::kVertex)
                    .starts_with("#version 450\n#define AYLO_INSTANCE_VALUES\n"));
    const std::string vertex = assemble(ShaderTarget::kSprite, ShaderStage::kVertex, "void fragment() {}\n");
    EXPECT_TRUE(vertex.starts_with("#version 450\n"));
    EXPECT_FALSE(has(vertex, "#define AYLO_INSTANCE_VALUES"));
}

TEST(ShaderAssembler, LeavesThePreludeAsItIsForTheDefaultShader) {
    for (const ShaderTarget target :
         {ShaderTarget::kSprite, ShaderTarget::kGlyph, ShaderTarget::kParticle, ShaderTarget::kShape}) {
        for (const ShaderStage stage : {ShaderStage::kVertex, ShaderStage::kFragment}) {
            const std::string glsl = assembleShader(ParsedShader{}, target, stage);
            EXPECT_TRUE(glsl.starts_with("#version 450\n"));
            // The markers stay, as in the built-in shaders that #include the prelude: GLSL ignores them.
            EXPECT_TRUE(has(glsl, "#pragma aylo_shader"));
            EXPECT_TRUE(has(glsl, "#pragma aylo_main"));
            EXPECT_FALSE(has(glsl, "#line"));
            EXPECT_FALSE(has(glsl, "#define AYLO_INSTANCE_VALUES"));
            EXPECT_FALSE(has(glsl, "vertex();"));
            EXPECT_FALSE(has(glsl, "fragment();"));
        }
    }
}

TEST(ShaderAssembler, AMeshReadsItsInstanceValuesFromItsDrawInBothStages) {
    const ParsedShader shader =
        parseShader("#pragma mesh\ninstance uniform float glow = 0.5;\nvoid fragment() { COLOR *= glow; }\n", "glow");
    for (const ShaderStage stage : {ShaderStage::kVertex, ShaderStage::kFragment}) {
        const std::string glsl = assembleShader(shader, ShaderTarget::kMesh, stage);
        EXPECT_NE(glsl.find("    glow = ayloMeshDraws[ayloPart.draw].instance[0].x;\n"), std::string::npos) << glsl;
        EXPECT_EQ(glsl.find("vec4 ayloInstance[4]"), std::string::npos) << glsl;
    }
}

TEST(ShaderAssembler, DeclaresTheVaryingsOutInTheVertexStageAndInInTheFragmentStage) {
    const ParsedShader shader =
        parseShader("varying vec2 wave;\nvarying int seed;\nvoid fragment() { COLOR.r = wave.x; }\n", "v");
    const std::string vertex = assembleShader(shader, ShaderTarget::kSprite, ShaderStage::kVertex);
    EXPECT_NE(vertex.find("layout(location = 7) out vec2 wave;\nlayout(location = 8) flat out int seed;\n"),
              std::string::npos)
        << vertex;
    const std::string fragment = assembleShader(shader, ShaderTarget::kShape, ShaderStage::kFragment);
    EXPECT_NE(fragment.find("layout(location = 11) in vec2 wave;\nlayout(location = 12) flat in int seed;\n"),
              std::string::npos)
        << fragment;
}

TEST(ShaderAssembler, CompilesTheCutoutInOnlyWhenAMeshShaderWritesIt) {
    const ParsedShader leaf = parseShader("#pragma mesh\nvoid fragment() { ALPHA_SCISSOR = 0.5; }\n", "leaf");
    EXPECT_TRUE(
        assembleShader(leaf, ShaderTarget::kMesh, ShaderStage::kFragment).contains("#define AYLO_ALPHA_SCISSOR"));
    const ParsedShader plain = parseShader("#pragma mesh\nvoid fragment() { COLOR.r = 1.0; }\n", "plain");
    EXPECT_FALSE(
        assembleShader(plain, ShaderTarget::kMesh, ShaderStage::kFragment).contains("#define AYLO_ALPHA_SCISSOR"));
}
