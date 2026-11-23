/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderIncludesTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "plugins/shader/detail/ShaderCompiler.hpp"
#include "plugins/shader/detail/ShaderIncludes.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"

using aylo::contract::renderer::CompiledShader;
using aylo::contract::renderer::ShaderTarget;
using aylo::plugins::shader::detail::compileShader;
using aylo::plugins::shader::detail::ExpandedShader;
using aylo::plugins::shader::detail::expandIncludes;
using aylo::plugins::shader::detail::IncludeReader;
using aylo::plugins::shader::detail::kMaxIncludeDepth;
using aylo::plugins::shader::exceptions::ShaderException;

namespace {
/// @return A reader over @p files, by path in the assets.
IncludeReader readerOf(std::map<std::string, std::string> files) {
    return [files = std::move(files)](const std::string& path) -> std::optional<std::string> {
        const auto found = files.find(path);
        return found != files.end() ? std::optional<std::string>{found->second} : std::nullopt;
    };
}

/// @return The message compiling @p source named "shaders/main.glsl" over @p read throws, empty if it compiles.
std::string errorOf(std::string_view source, const IncludeReader& read) {
    try {
        static_cast<void>(compileShader(source, "shaders/main.glsl", {}, read));
    } catch (const ShaderException& error) {
        return error.what();
    }
    return {};
}
}  // namespace

TEST(ShaderIncludes, ReplacesAnIncludeByTheFileAndKnowsWhereEachLineComesFrom) {
    const ExpandedShader expanded =
        expandIncludes("// main\n#include \"lib.glsl\"\nvoid fragment() {}", "shaders/main.glsl",
                       readerOf({{"shaders/lib.glsl", "float twice(float x) { return x * 2.0; }"}}));
    EXPECT_EQ(expanded.text, "// main\n\nfloat twice(float x) { return x * 2.0; }\nvoid fragment() {}");
    EXPECT_EQ(expanded.files, (std::vector<std::string>{"shaders/main.glsl", "shaders/lib.glsl"}));
    EXPECT_EQ(expanded.locate(3), "line 1 of 'shaders/lib.glsl'");
    EXPECT_EQ(expanded.locate(4), "line 3");
    EXPECT_EQ(expanded.locateLines("Shader 'x' (sprite fragment): line 3: oops\n  line 4: again"),
              "Shader 'x' (sprite fragment): line 1 of 'shaders/lib.glsl': oops\n  line 3: again");
}

TEST(ShaderIncludes, LooksNextToTheFileThenFromTheAssets) {
    const IncludeReader read = readerOf({{"common/a.glsl", "#include \"b.glsl\"\n"}, {"common/b.glsl", "// b\n"}});
    const ExpandedShader expanded = expandIncludes("#include \"common/a.glsl\"\n", "shaders/main.glsl", read);
    EXPECT_EQ(expanded.files, (std::vector<std::string>{"shaders/main.glsl", "common/a.glsl", "common/b.glsl"}));
}

TEST(ShaderIncludes, IncludesAFileOnceAndRefusesACycle) {
    const IncludeReader read = readerOf({{"shaders/a.glsl", "#include \"c.glsl\"\n"},
                                         {"shaders/b.glsl", "#include \"c.glsl\"\n"},
                                         {"shaders/c.glsl", "float c() { return 1.0; }\n"}});
    const ExpandedShader twice =
        expandIncludes("#include \"a.glsl\"\n#include \"b.glsl\"\n", "shaders/main.glsl", read);
    EXPECT_EQ(std::ranges::count(twice.files, std::string{"shaders/c.glsl"}), 1);

    const IncludeReader cycle =
        readerOf({{"shaders/a.glsl", "\n#include \"b.glsl\"\n"}, {"shaders/b.glsl", "#include \"a.glsl\"\n"}});
    const std::string message = errorOf("#include \"a.glsl\"\nvoid fragment() {}\n", cycle);
    EXPECT_EQ(message, "Shader 'shaders/main.glsl', line 1 of 'shaders/b.glsl': 'shaders/a.glsl' includes itself");
}

TEST(ShaderIncludes, ReportsAMissingFileAndTooDeepAChain) {
    const std::string missing = errorOf("\n#include \"nothing.glsl\"\nvoid fragment() {}\n", readerOf({}));
    EXPECT_EQ(missing,
              "Shader 'shaders/main.glsl', line 2: cannot find \"nothing.glsl\" (looked for "
              "'shaders/nothing.glsl', 'nothing.glsl')");
    EXPECT_TRUE(errorOf("#include nothing.glsl\nvoid fragment() {}\n", readerOf({})).contains("expected #include"));
    EXPECT_TRUE(errorOf("#include \"@aylo/nothing.glsl\"\nvoid fragment() {}\n", readerOf({}))
                    .contains("the engine has no library '@aylo/nothing.glsl'"));

    std::map<std::string, std::string> chain;
    for (std::size_t level = 0; level <= kMaxIncludeDepth; ++level) {
        chain[std::format("shaders/{}.glsl", level)] = std::format("#include \"{}.glsl\"\n", level + 1);
    }
    EXPECT_TRUE(errorOf("#include \"0.glsl\"\nvoid fragment() {}\n", readerOf(chain)).contains("levels of #include"));
}

TEST(ShaderIncludes, AnErrorInAnIncludedFileNamesThatFile) {
    const IncludeReader read = readerOf({{"shaders/lib.glsl", "// lib\nfloat broken() { return nothing; }\n"},
                                         {"shaders/values.glsl", "\nuniform float DEPTH_TEXTURE;\n"}});
    const std::string glslang = errorOf("#include \"lib.glsl\"\nvoid fragment() {}\n", read);
    EXPECT_TRUE(glslang.contains("line 2 of 'shaders/lib.glsl': 'nothing' : undeclared identifier")) << glslang;
    const std::string parser = errorOf("\n#include \"values.glsl\"\nvoid fragment() {}\n", read);
    EXPECT_TRUE(parser.starts_with("Shader 'shaders/main.glsl', line 2 of 'shaders/values.glsl': ")) << parser;
}

TEST(ShaderIncludes, DeclarationsInAnIncludedFileBelongToTheShader) {
    const IncludeReader read = readerOf(
        {{"shaders/tint.glsl", "uniform vec4 tint = vec4(1.0, 0.0, 0.0, 1.0);\nuniform sampler2D pattern;\n"}});
    std::vector<std::string> included;
    const CompiledShader shader =
        compileShader("#include \"tint.glsl\"\nvoid fragment() { COLOR *= tint * texture(pattern, UV); }\n",
                      "shaders/main.glsl", {}, read, &included);
    ASSERT_EQ(shader.materialValues.size(), 1U);
    EXPECT_EQ(shader.materialValues.front().name, "tint");
    ASSERT_EQ(shader.textures.size(), 1U);
    EXPECT_EQ(included, std::vector<std::string>{"shaders/tint.glsl"});
}

TEST(ShaderIncludes, TheEnginesLibrariesCompileInEveryTarget) {
    // The issue's example, without the game file: what each library gives, used in each kind of shader.
    constexpr std::string_view kUses = R"(
#include "@aylo/noise.glsl"
#include "@aylo/color.glsl"
#include "@aylo/math.glsl"
float everything(vec4 p) {
    float n = whiteNoise(p.x) + whiteNoise(vec2(p.x, p.y)) + whiteNoise(vec3(p.x, p.y, p.z)) + whiteNoise(p);
    n += valueNoise(p.x) + valueNoise(vec2(p.x, p.y)) + valueNoise(vec3(p.x, p.y, p.z)) + valueNoise(p);
    n += perlinNoise(p.x) + perlinNoise(vec2(p.x, p.y)) + perlinNoise(vec3(p.x, p.y, p.z)) + perlinNoise(p);
    n += simplexNoise(p.x) + simplexNoise(vec2(p.x, p.y)) + simplexNoise(vec3(p.x, p.y, p.z)) + simplexNoise(p);
    n += fbm(p.x, 4, 0.5, 2.0) + fbm(vec2(p.x, p.y), 4, 0.5, 2.0) + fbm(vec3(p.x, p.y, p.z), 4, 0.5, 2.0);
    n += fbm(p, 4, 0.5, 2.0) + voronoi(vec2(p.x, p.y)).x + voronoi(vec3(p.x, p.y, p.z), VORONOI_MINKOWSKI, 3.0).z;
    vec3 c = blendOverlay(vec3(n), hsvToRgb(rgbToHsv(vec3(0.2))), 0.7) + blendSoftLight(vec3(0.5), vec3(n), 1.0);
    c = blendScreen(c, blendMultiply(c, c, 0.5), 0.5) + blendLinearLight(c, c, 0.2) + blendAdd(c, c, 0.1);
    c = blendSubtract(c, blendDifference(c, blendDarken(c, blendLighten(c, c, 1.0), 1.0), 1.0), 0.5);
    c = blendColorDodge(c, blendColorBurn(c, c, 1.0), 0.5) + linearToSrgb(srgbToLinear(c));
    vec2 turned = rotate(vec2(n), PI) + remap(vec2(1.0), vec2(0.0), vec2(2.0), vec2(0.0), vec2(1.0));
    return luminance(c) + turned.x + rotate(vec3(n), vec3(0.0, 1.0, 0.0), TAU).y + random(7u, 3u) + random(n) +
           remapClamped(n, 0.0, 1.0, 1.0, 0.0) + float(hashBits(uvec4(1u))) * 0.0;
}
)";
    const std::string drawing =
        std::string{kUses} + "void fragment() { COLOR.rgb *= everything(vec4(UV, TIME, 1.0)); }\n";
    EXPECT_EQ(compileShader(drawing, "drawing").variants.size(), 4U);
    const std::string mesh = std::string{"#pragma mesh\n"} + std::string{kUses} +
                             "void fragment() { COLOR.rgb *= everything(vec4(WORLD_POSITION, TIME)); }\n";
    EXPECT_EQ(compileShader(mesh, "mesh").variants.size(), 1U);
    const std::string lit = std::string{"#pragma mesh\n#pragma lit\n"} + std::string{kUses} +
                            "void fragment() { ALBEDO = vec3(everything(vec4(WORLD_POSITION, 0.0))); }\n";
    EXPECT_EQ(compileShader(lit, "lit").variants.size(), 1U);
    const std::string screen = std::string{"#pragma screen\n"} + std::string{kUses} +
                               "void fragment() { COLOR.rgb *= everything(vec4(SCREEN_UV, TIME, 0.0)); }\n";
    EXPECT_EQ(compileShader(screen, "screen").variants.front().target, ShaderTarget::kScreen);

    // manual/pages/shader-libraries.md, "Cracked stone".
    const CompiledShader stone = compileShader(R"(#pragma mesh
#pragma lit
#include "@aylo/noise.glsl"
void fragment() {
    vec3 cells = voronoi(WORLD_POSITION * 3.0);
    float crack = smoothstep(0.0, 0.05, cells.z);  // dark on the borders
    float grain = fbm(WORLD_POSITION * 12.0, 4, 0.5, 2.0) * 0.5 + 0.5;
    ALBEDO = vec3(0.45, 0.43, 0.4) * mix(0.8, 1.1, grain) * mix(0.2, 1.0, crack);
    ROUGHNESS = 0.9;
}
)",
                                               "stone");
    EXPECT_EQ(stone.variants.size(), 1U);
}
