/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderLibrariesTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <functional>

#include "GlslShim.hpp"

// The engine's libraries, as C++: the same files the shaders include.
// NOLINTBEGIN(readability-*, misc-*, cppcoreguidelines-*, bugprone-*, google-*, modernize-*, performance-*)
namespace aylo::test::glsl {
#include "plugins/shader/builtin/color.glsl"
#include "plugins/shader/builtin/math.glsl"
#include "plugins/shader/builtin/noise.glsl"
}  // namespace aylo::test::glsl
// NOLINTEND(readability-*, misc-*, cppcoreguidelines-*, bugprone-*, google-*, modernize-*, performance-*)

namespace glsl = aylo::test::glsl;

namespace {
/// @brief What a noise does over a grid of positions.
struct Sampled {
    float low{1e9F};     ///< Its lowest value.
    float high{-1e9F};   ///< Its highest.
    float mean{0.0F};    ///< Its mean.
    float spread{0.0F};  ///< Its standard deviation.
    float jump{0.0F};    ///< The largest change for a step of 0.001 along every axis.
};

/// @return @p noise sampled at 4096 positions spread over many cells, each compared with a position 0.001 away.
Sampled sample(const std::function<float(glsl::vec4)>& noise) {
    Sampled sampled;
    double sum = 0.0;
    double squares = 0.0;
    constexpr int kCount = 4096;
    for (int index = 0; index < kCount; ++index) {
        const glsl::vec4 position{static_cast<float>(index % 64) * 0.37F - 11.0F,
                                  static_cast<float>(index / 64) * 0.41F - 13.0F, static_cast<float>(index) * 0.013F,
                                  static_cast<float>(index % 7) * 0.29F};
        const float value = noise(position);
        sampled.low = std::min(sampled.low, value);
        sampled.high = std::max(sampled.high, value);
        sum += value;
        squares += static_cast<double>(value) * value;
        sampled.jump = std::max(sampled.jump, std::abs(noise(position + glsl::vec4{0.001F}) - value));
    }
    sampled.mean = static_cast<float>(sum / kCount);
    sampled.spread = static_cast<float>(std::sqrt((squares / kCount) - (sampled.mean * sampled.mean)));
    return sampled;
}

/// @brief Checks a noise centred on 0: in [-1, 1], using a good part of it, continuous.
void expectSignedNoise(const Sampled& sampled) {
    EXPECT_GE(sampled.low, -1.0F);
    EXPECT_LE(sampled.high, 1.0F);
    EXPECT_GT(sampled.high - sampled.low, 0.8F);
    EXPECT_LT(std::abs(sampled.mean), 0.08F);
    EXPECT_GT(sampled.spread, 0.1F);
    EXPECT_LT(sampled.jump, 0.03F);
}

glsl::vec2 xy(glsl::vec4 p) { return {p.x, p.y}; }
glsl::vec3 xyz(glsl::vec4 p) { return {p.x, p.y, p.z}; }
}  // namespace

TEST(ShaderLibraries, WhiteNoiseIsUniformAndRepeatable) {
    const Sampled sampled = sample([](glsl::vec4 p) { return glsl::whiteNoise(p); });
    EXPECT_GE(sampled.low, 0.0F);
    EXPECT_LT(sampled.high, 1.0F);
    EXPECT_NEAR(sampled.mean, 0.5F, 0.02F);
    EXPECT_NEAR(sampled.spread, 0.2887F, 0.02F);  // 1 / sqrt(12), a uniform distribution's
    EXPECT_EQ(glsl::whiteNoise(glsl::vec2{1.5F, 2.5F}), glsl::whiteNoise(glsl::vec2{1.5F, 2.5F}));
    EXPECT_NE(glsl::random(1U), glsl::random(2U));
    EXPECT_EQ(glsl::random(7U, 3U), glsl::random(7U, 3U));
    EXPECT_NE(glsl::random(7U, 3U), glsl::random(7U, 4U));
}

TEST(ShaderLibraries, ValueNoiseHitsTheLatticeValuesAndIsContinuous) {
    for (const auto& noise : std::initializer_list<std::function<float(glsl::vec4)>>{
             [](glsl::vec4 p) { return glsl::valueNoise(p.x); }, [](glsl::vec4 p) { return glsl::valueNoise(xy(p)); },
             [](glsl::vec4 p) { return glsl::valueNoise(xyz(p)); }, [](glsl::vec4 p) { return glsl::valueNoise(p); }}) {
        const Sampled sampled = sample(noise);
        EXPECT_GE(sampled.low, 0.0F);
        EXPECT_LE(sampled.high, 1.0F);
        EXPECT_NEAR(sampled.mean, 0.5F, 0.08F);
        EXPECT_LT(sampled.jump, 0.03F);
    }
    EXPECT_FLOAT_EQ(glsl::valueNoise(glsl::vec2{3.0F, -2.0F}),
                    glsl::hashToFloat(glsl::hashBits(glsl::uvec2(glsl::ivec2{3, -2}))));
}

TEST(ShaderLibraries, PerlinNoiseIsZeroOnTheLatticeAndSpansMinusOneToOne) {
    expectSignedNoise(sample([](glsl::vec4 p) { return glsl::perlinNoise(p.x); }));
    expectSignedNoise(sample([](glsl::vec4 p) { return glsl::perlinNoise(xy(p)); }));
    expectSignedNoise(sample([](glsl::vec4 p) { return glsl::perlinNoise(xyz(p)); }));
    expectSignedNoise(sample([](glsl::vec4 p) { return glsl::perlinNoise(p); }));
    EXPECT_FLOAT_EQ(glsl::perlinNoise(4.0F), 0.0F);
    EXPECT_NEAR(glsl::perlinNoise(glsl::vec2{2.0F, -5.0F}), 0.0F, 1e-6F);
    EXPECT_NEAR(glsl::perlinNoise(glsl::vec3{1.0F, 2.0F, 3.0F}), 0.0F, 1e-6F);
    EXPECT_NEAR(glsl::perlinNoise(glsl::vec4{-1.0F, 0.0F, 7.0F, 2.0F}), 0.0F, 1e-6F);
}

TEST(ShaderLibraries, SimplexNoiseSpansMinusOneToOneAndIsContinuous) {
    expectSignedNoise(sample([](glsl::vec4 p) { return glsl::simplexNoise(p.x); }));
    expectSignedNoise(sample([](glsl::vec4 p) { return glsl::simplexNoise(xy(p)); }));
    expectSignedNoise(sample([](glsl::vec4 p) { return glsl::simplexNoise(xyz(p)); }));
    expectSignedNoise(sample([](glsl::vec4 p) { return glsl::simplexNoise(p); }));
}

TEST(ShaderLibraries, FbmStaysInRangeAndAddsDetail) {
    const Sampled smooth = sample([](glsl::vec4 p) { return glsl::fbm(xyz(p), 1, 0.5F, 2.0F); });
    const Sampled detailed = sample([](glsl::vec4 p) { return glsl::fbm(xyz(p), 6, 0.5F, 2.0F); });
    EXPECT_GE(detailed.low, -1.0F);
    EXPECT_LE(detailed.high, 1.0F);
    EXPECT_GT(detailed.jump, smooth.jump);  // finer octaves change faster
    EXPECT_FLOAT_EQ(glsl::fbm(glsl::vec2{0.3F, 0.7F}, 0, 0.5F, 2.0F), 0.0F);
}

TEST(ShaderLibraries, VoronoiMeasuresTheNearestPoints) {
    for (int index = 0; index < 512; ++index) {
        const glsl::vec2 position{static_cast<float>(index % 32) * 0.31F, static_cast<float>(index / 32) * 0.29F};
        const glsl::vec3 euclidean = glsl::voronoi(position);
        EXPECT_LE(euclidean.x, euclidean.y);
        EXPECT_FLOAT_EQ(euclidean.z, euclidean.y - euclidean.x);
        EXPECT_LT(euclidean.x, 1.5F);
        const float manhattan = glsl::voronoi(position, glsl::VORONOI_MANHATTAN, 1.0F).x;
        const float chebyshev = glsl::voronoi(position, glsl::VORONOI_CHEBYSHEV, 1.0F).x;
        EXPECT_LE(chebyshev, euclidean.x + 1e-5F);
        EXPECT_LE(euclidean.x, manhattan + 1e-5F);
        EXPECT_NEAR(glsl::voronoi(position, glsl::VORONOI_MINKOWSKI, 1.0F).x, manhattan, 1e-4F);
        EXPECT_NEAR(glsl::voronoi(position, glsl::VORONOI_MINKOWSKI, 2.0F).x, euclidean.x, 1e-4F);
    }
    // On the point of its cell, F1 is 0.
    const glsl::uint bits = glsl::hashBits(glsl::uvec3(glsl::uvec2(glsl::ivec2{2, 3}), 7U));
    const glsl::vec2 feature =
        glsl::vec2{2.0F, 3.0F} + glsl::vec2{glsl::hashToFloat(bits), glsl::hashToFloat(glsl::hashBits(bits))};
    EXPECT_NEAR(glsl::voronoi(feature).x, 0.0F, 1e-6F);
    const glsl::vec3 inSpace = glsl::voronoi(glsl::vec3{0.5F, 1.5F, -2.5F});
    EXPECT_LE(inSpace.x, inSpace.y);
}

TEST(ShaderLibraries, ColorSpacesFollowTheirFormulas) {
    EXPECT_NEAR(glsl::srgbToLinear(0.5F), 0.21404F, 1e-4F);
    EXPECT_NEAR(glsl::linearToSrgb(0.21404F), 0.5F, 1e-4F);
    EXPECT_NEAR(glsl::srgbToLinear(0.02F), 0.02F / 12.92F, 1e-6F);
    for (float channel = 0.0F; channel <= 1.0F; channel += 0.05F) {
        EXPECT_NEAR(glsl::linearToSrgb(glsl::srgbToLinear(channel)), channel, 1e-4F);
    }
    EXPECT_FLOAT_EQ(glsl::luminance(glsl::vec3{1.0F}), 1.0F);
    EXPECT_FLOAT_EQ(glsl::linearToSrgb(glsl::vec4{0.0F, 0.0F, 0.0F, 0.25F}).w, 0.25F);

    const glsl::vec3 red = glsl::hsvToRgb(glsl::vec3{0.0F, 1.0F, 1.0F});
    EXPECT_NEAR(glm::distance(red, glsl::vec3{1.0F, 0.0F, 0.0F}), 0.0F, 1e-5F);
    const glsl::vec3 green = glsl::hsvToRgb(glsl::vec3{1.0F / 3.0F, 1.0F, 1.0F});
    EXPECT_NEAR(glm::distance(green, glsl::vec3{0.0F, 1.0F, 0.0F}), 0.0F, 1e-5F);
    for (const glsl::vec3 color : {glsl::vec3{0.2F, 0.4F, 0.9F}, glsl::vec3{0.9F, 0.1F, 0.3F},
                                   glsl::vec3{0.5F, 0.5F, 0.5F}, glsl::vec3{0.1F, 0.8F, 0.2F}}) {
        EXPECT_NEAR(glm::distance(glsl::hsvToRgb(glsl::rgbToHsv(color)), color), 0.0F, 1e-5F);
    }
    EXPECT_NEAR(glsl::rgbToHsv(glsl::vec3{0.0F, 0.0F, 1.0F}).x, 2.0F / 3.0F, 1e-6F);
}

TEST(ShaderLibraries, BlendModesFollowTheirFormulas) {
    const glsl::vec3 base{0.2F, 0.6F, 0.9F};
    const glsl::vec3 blend{0.5F, 0.3F, 0.8F};
    const auto near = [](glsl::vec3 actual, glsl::vec3 expected) {
        EXPECT_NEAR(glm::distance(actual, expected), 0.0F, 1e-5F) << actual.x << " " << actual.y << " " << actual.z;
    };
    near(glsl::blendMultiply(base, blend, 1.0F), base * blend);
    near(glsl::blendScreen(base, blend, 1.0F),
         glsl::vec3{1.0F} - ((glsl::vec3{1.0F} - base) * (glsl::vec3{1.0F} - blend)));
    near(glsl::blendOverlay(base, blend, 1.0F),
         glsl::vec3{2.0F * 0.2F * 0.5F, 1.0F - (2.0F * 0.4F * 0.7F), 1.0F - (2.0F * 0.1F * 0.2F)});
    near(glsl::blendSoftLight(base, glsl::vec3{0.5F}, 1.0F), base);  // a mid-grey blend leaves it as it is
    near(glsl::blendLinearLight(base, blend, 1.0F), base + (2.0F * blend) - glsl::vec3{1.0F});
    near(glsl::blendAdd(base, blend, 1.0F), base + blend);
    near(glsl::blendSubtract(base, blend, 1.0F), glsl::vec3{0.0F, 0.3F, 0.1F});
    near(glsl::blendDifference(base, blend, 1.0F), glsl::vec3{0.3F, 0.3F, 0.1F});
    near(glsl::blendDarken(base, blend, 1.0F), glsl::vec3{0.2F, 0.3F, 0.8F});
    near(glsl::blendLighten(base, blend, 1.0F), glsl::vec3{0.5F, 0.6F, 0.9F});
    near(glsl::blendColorDodge(base, blend, 1.0F), glsl::vec3{0.4F, 0.6F / 0.7F, 1.0F});
    near(glsl::blendColorBurn(base, blend, 1.0F), glsl::vec3{0.0F, 0.0F, 1.0F - (0.1F / 0.8F)});
    near(glsl::blendMultiply(base, blend, 0.0F), base);
    near(glsl::blendScreen(base, blend, 0.5F), glm::mix(base, glsl::blendScreen(base, blend, 1.0F), 0.5F));
}

TEST(ShaderLibraries, MathRemapsAndRotates) {
    EXPECT_FLOAT_EQ(glsl::remap(5.0F, 0.0F, 10.0F, 0.0F, 1.0F), 0.5F);
    EXPECT_FLOAT_EQ(glsl::remap(15.0F, 0.0F, 10.0F, 0.0F, 1.0F), 1.5F);
    EXPECT_FLOAT_EQ(glsl::remapClamped(15.0F, 0.0F, 10.0F, 1.0F, 0.0F), 0.0F);
    const glsl::vec2 turned = glsl::rotate(glsl::vec2{1.0F, 0.0F}, glsl::PI / 2.0F);
    EXPECT_NEAR(turned.x, 0.0F, 1e-6F);
    EXPECT_NEAR(turned.y, 1.0F, 1e-6F);
    const glsl::vec3 around = glsl::rotate(glsl::vec3{1.0F, 0.0F, 0.0F}, glsl::vec3{0.0F, 0.0F, 2.0F}, glsl::PI / 2.0F);
    EXPECT_NEAR(glm::distance(around, glsl::vec3{0.0F, 1.0F, 0.0F}), 0.0F, 1e-6F);
    for (glsl::uint seed = 0; seed < 1000; ++seed) {
        const float value = glsl::random(seed);
        EXPECT_GE(value, 0.0F);
        EXPECT_LT(value, 1.0F);
    }
}
