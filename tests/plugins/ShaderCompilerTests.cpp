/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderCompilerTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "plugins/shader/detail/ShaderAssembler.hpp"
#include "plugins/shader/detail/ShaderCompiler.hpp"
#include "plugins/shader/detail/ShaderParser.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"

using aylo::contract::renderer::CompiledShader;
using aylo::contract::renderer::ShaderTarget;
using aylo::plugins::shader::detail::assembleShader;
using aylo::plugins::shader::detail::compileShader;
using aylo::plugins::shader::detail::compileStage;
using aylo::plugins::shader::detail::ParsedShader;
using aylo::plugins::shader::detail::ShaderStage;
using aylo::plugins::shader::exceptions::ShaderException;

namespace {
/// @brief The first word of every SPIR-V module.
constexpr std::uint32_t kSpirvMagic = 0x07230203;

/// @brief A shader using every feature: material and instance values, a texture, both functions.
constexpr std::string_view kFlash =
    "uniform vec4 tint = vec4(1.0, 0.2, 0.2, 1.0);\n"
    "instance uniform float flash;\n"
    "uniform sampler2D noise;\n"
    "void vertex() { VERTEX.y += sin(TIME); }\n"
    "void fragment() {\n"
    "    if (texture(noise, UV).r < 0.05) { discard; }\n"
    "    COLOR.rgb = mix(COLOR.rgb, tint.rgb * COLOR.a, flash);\n"
    "}\n";

/// @return The message of the ShaderException compiling @p source throws, or an empty string if it throws none.
std::string errorOf(std::string_view source) {
    try {
        static_cast<void>(compileShader(source, "test"));
    } catch (const ShaderException& error) {
        return error.what();
    }
    return {};
}
}  // namespace

TEST(ShaderCompiler, CompilesAShaderForEveryTarget) {
    const CompiledShader shader = compileShader(kFlash, "flash");
    ASSERT_EQ(shader.variants.size(), 4U);
    for (const ShaderTarget target : aylo::plugins::shader::detail::kDrawingTargets) {
        const auto* variant = shader.findVariant(target);
        ASSERT_NE(variant, nullptr);
        ASSERT_FALSE(variant->vertex.empty());
        ASSERT_FALSE(variant->fragment.empty());
        EXPECT_EQ(variant->vertex.front(), kSpirvMagic);
        EXPECT_EQ(variant->fragment.front(), kSpirvMagic);
    }
    EXPECT_NO_THROW(aylo::contract::renderer::validate(shader));
}

TEST(ShaderCompiler, CarriesWhatTheParserFound) {
    const CompiledShader shader = compileShader(kFlash, "flash");
    EXPECT_EQ(shader.name, "flash");
    ASSERT_EQ(shader.materialValues.size(), 1U);
    EXPECT_EQ(shader.materialValues.front().name, "tint");
    EXPECT_EQ(shader.materialSize, 16U);
    EXPECT_EQ(shader.materialDefaults.size(), 16U);
    ASSERT_EQ(shader.instanceValues.size(), 1U);
    EXPECT_EQ(shader.instanceValues.front().name, "flash");
    ASSERT_EQ(shader.textures.size(), 1U);
    EXPECT_EQ(shader.textures.front().name, "noise");
}

TEST(ShaderCompiler, CompilesOnlyTheTargetsAskedFor) {
    const std::array targets{ShaderTarget::kParticle};
    const CompiledShader shader = compileShader("void fragment() {}\n", "minimal", targets);
    ASSERT_EQ(shader.variants.size(), 1U);
    EXPECT_EQ(shader.variants.front().target, ShaderTarget::kParticle);
}

TEST(ShaderCompiler, ReportsAnErrorAtTheShadersOwnLine) {
    const std::string message = errorOf("uniform float a;\n\nvoid fragment() {\n    COLOR = notDeclared;\n}\n");
    EXPECT_TRUE(message.starts_with("Shader 'test' (sprite fragment): line 4: 'notDeclared'")) << message;
}

TEST(ShaderCompiler, PassesTheParsersErrorsOn) {
    EXPECT_NE(errorOf("uniform mat4 m;\nvoid fragment() {}\n").find("line 1"), std::string::npos);
}

TEST(ShaderCompiler, RejectsInvalidGlslForAStage) {
    EXPECT_THROW(static_cast<void>(compileStage("#version 450\nvoid main() { oops }\n", ShaderStage::kFragment, "x")),
                 ShaderException);
}

TEST(ShaderCompiler, CompilesTheDefaultShaderOfEveryTarget) {
    for (const ShaderTarget target : aylo::plugins::shader::detail::kDrawingTargets) {
        for (const ShaderStage stage : {ShaderStage::kVertex, ShaderStage::kFragment}) {
            const std::vector<std::uint32_t> spirv =
                compileStage(assembleShader(ParsedShader{}, target, stage), stage, "default");
            ASSERT_FALSE(spirv.empty());
            EXPECT_EQ(spirv.front(), kSpirvMagic);
        }
    }
}

TEST(ShaderCompiler, CompilesAScreenShaderForTheScreenOnly) {
    const CompiledShader shader = compileShader(
        "#pragma screen\n"
        "uniform float strength = 0.5;\n"
        "void fragment() {\n"
        "    vec3 left = texture(SCREEN_TEXTURE, SCREEN_UV - vec2(SCREEN_PIXEL_SIZE.x, 0.0)).rgb;\n"
        "    COLOR.rgb = mix(COLOR.rgb, left, strength);\n"
        "}\n",
        "blur");
    ASSERT_EQ(shader.variants.size(), 1U);
    EXPECT_EQ(shader.variants.front().target, ShaderTarget::kScreen);
    EXPECT_EQ(shader.variants.front().fragment.front(), kSpirvMagic);
}

TEST(ShaderCompiler, GivesAScreenShaderItsInstanceDefaultsAsConstants) {
    const CompiledShader shader = compileShader(
        "#pragma screen\ninstance uniform vec2 shift = vec2(0.25, 1.0);\ninstance uniform int mode = 3;\n"
        "void fragment() { COLOR.rg += shift * float(mode); }\n",
        "shifted");
    ASSERT_EQ(shader.variants.size(), 1U);
    const std::string vertex =
        assembleShader(aylo::plugins::shader::detail::parseShader(
                           "#pragma screen\ninstance uniform int mode = 3;\nvoid fragment() {}\n", "mode"),
                       ShaderTarget::kScreen, ShaderStage::kVertex);
    EXPECT_NE(vertex.find("    ayloInstance = vec4[4](uintBitsToFloat(uvec4(3u, 0u, 0u, 0u))"), std::string::npos)
        << vertex;
}

TEST(ShaderCompiler, TheScreenBuiltinsAreOnlyForScreenShaders) {
    const std::string message = errorOf("void fragment() { COLOR = texture(SCREEN_TEXTURE, SCREEN_UV); }\n");
    EXPECT_NE(message.find("SCREEN_TEXTURE"), std::string::npos) << message;
}

TEST(ShaderCompiler, CompilesTheDefaultScreenShader) {
    for (const ShaderStage stage : {ShaderStage::kVertex, ShaderStage::kFragment}) {
        const std::vector<std::uint32_t> spirv =
            compileStage(assembleShader(ParsedShader{}, ShaderTarget::kScreen, stage), stage, "default");
        ASSERT_FALSE(spirv.empty());
        EXPECT_EQ(spirv.front(), kSpirvMagic);
    }
}

TEST(ShaderCompiler, CompilesTheManualsScreenEffects) {
    // manual/pages/post-processing.md: "Writing a screen effect" and "The whole game in ASCII art".
    constexpr std::string_view kGrayscale =
        "#pragma screen\n"
        "uniform float amount = 1.0;\n"
        "void fragment() {\n"
        "    float gray = dot(COLOR.rgb, vec3(0.299, 0.587, 0.114));\n"
        "    COLOR.rgb = mix(COLOR.rgb, vec3(gray), amount);\n"
        "}\n";
    constexpr std::string_view kAscii =
        "#pragma screen\n"
        "uniform float cellSize = 8.0;\n"
        "void fragment() {\n"
        "    vec2 cell = cellSize * SCREEN_PIXEL_SIZE;\n"
        "    vec2 center = (floor(SCREEN_UV / cell) + 0.5) * cell;\n"
        "    float brightness = dot(texture(SCREEN_TEXTURE, center).rgb, vec3(0.299, 0.587, 0.114));\n"
        "    float mark = step(length(fract(SCREEN_UV / cell) - 0.5), 0.5 * clamp(brightness, 0.0, 1.0));\n"
        "    COLOR = vec4(vec3(0.2, 1.0, 0.3) * mark, 1.0);\n"
        "}\n";
    for (const std::string_view source : {kGrayscale, kAscii}) {
        EXPECT_EQ(errorOf(source), "") << source;
    }
}
