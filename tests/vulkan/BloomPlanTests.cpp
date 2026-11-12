/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BloomPlanTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <glm/ext/vector_uint2.hpp>

#include "aylo/contract/renderer/postprocess/PostProcess.hpp"
#include "render/vulkan/postprocess/BloomPlan.hpp"

namespace postprocess = aylo::render::vulkan::postprocess;
namespace renderer = aylo::contract::renderer;

TEST(BloomPlan, AFullHdWindowGetsTheLongestChain) {
    EXPECT_EQ(postprocess::bloomLevelCount({1920, 1080}), postprocess::kMaxBloomLevels);
}

TEST(BloomPlan, TheChainStopsBeforeAnImageGetsThinnerThanFourPixels) {
    for (const std::uint32_t side : {16U, 40U, 100U, 333U, 1080U, 4096U}) {
        const std::uint32_t levels = postprocess::bloomLevelCount({side, side});
        ASSERT_GE(levels, 1U);
        ASSERT_LE(levels, postprocess::kMaxBloomLevels);
        const glm::uvec2 last = postprocess::bloomLevelSize({side, side}, levels - 1);
        if (levels > 1) {
            EXPECT_GE(last.x, 4U) << "a " << side << " pixels scene";
        }
        if (levels < postprocess::kMaxBloomLevels) {
            EXPECT_LT(postprocess::bloomLevelSize({side, side}, levels).x, 4U) << "a " << side << " pixels scene";
        }
    }
}

TEST(BloomPlan, ATinyWindowStillHasOneLevel) {
    EXPECT_EQ(postprocess::bloomLevelCount({1, 1}), 1U);
    EXPECT_EQ(postprocess::bloomLevelCount({0, 0}), 1U);
    EXPECT_EQ(postprocess::bloomLevelCount({2, 3000}), 1U);
}

TEST(BloomPlan, TheThinnerSideDecidesTheLength) {
    EXPECT_EQ(postprocess::bloomLevelCount({4000, 64}), postprocess::bloomLevelCount({64, 64}));
}

TEST(BloomPlan, EachLevelIsHalfThePreviousOneAndNeverEmpty) {
    const glm::uvec2 scene{1920, 1080};
    EXPECT_EQ(postprocess::bloomLevelSize(scene, 0), (glm::uvec2{960, 540}));
    EXPECT_EQ(postprocess::bloomLevelSize(scene, 1), (glm::uvec2{480, 270}));
    EXPECT_EQ(postprocess::bloomLevelSize(scene, 5), (glm::uvec2{30, 16}));
    EXPECT_EQ(postprocess::bloomLevelSize({1, 1}, 0), (glm::uvec2{1, 1}));
    EXPECT_EQ(postprocess::bloomLevelSize({3, 100}, 4), (glm::uvec2{1, 3}));
}

TEST(BloomPlan, ThePrefilterReadsTheSceneWithTheThresholdAndAKneeInBrightness) {
    const renderer::BloomSettings bloom{.enabled = true, .threshold = 2.0F, .softKnee = 0.25F, .intensity = 0.5F};
    const postprocess::PostConstants constants = postprocess::makePrefilterConstants({200, 100}, bloom);
    EXPECT_FLOAT_EQ(constants.texelSize.x, 1.0F / 200.0F);
    EXPECT_FLOAT_EQ(constants.texelSize.y, 1.0F / 100.0F);
    EXPECT_FLOAT_EQ(constants.threshold, 2.0F);
    EXPECT_FLOAT_EQ(constants.knee, 0.5F);
    EXPECT_EQ(constants.flags & postprocess::kFlagPrefilter, postprocess::kFlagPrefilter);
}

TEST(BloomPlan, ThePlainFiltersDoNotPrefilter) {
    const postprocess::PostConstants constants = postprocess::makeFilterConstants({64, 32});
    EXPECT_FLOAT_EQ(constants.texelSize.x, 1.0F / 64.0F);
    EXPECT_EQ(constants.flags, 0U);
}

TEST(BloomPlan, TheCompositeAddsNoGlowWhenBloomIsOff) {
    renderer::PostProcessSettings settings;
    settings.bloom.intensity = 0.9F;
    settings.exposure = 1.5F;
    settings.tonemap = renderer::Tonemap::kAces;
    EXPECT_FLOAT_EQ(postprocess::makeCompositeConstants(settings).intensity, 0.0F);

    settings.bloom.enabled = true;
    const postprocess::PostConstants on = postprocess::makeCompositeConstants(settings);
    EXPECT_FLOAT_EQ(on.intensity, 0.9F);
    EXPECT_FLOAT_EQ(on.exposure, 1.5F);
    EXPECT_EQ(on.tonemap, static_cast<std::int32_t>(renderer::Tonemap::kAces));
}

TEST(BloomPlan, TheTonemapsHaveTheNumbersTheShaderReads) {
    EXPECT_EQ(static_cast<int>(renderer::Tonemap::kNone), 0);
    EXPECT_EQ(static_cast<int>(renderer::Tonemap::kReinhard), 1);
    EXPECT_EQ(static_cast<int>(renderer::Tonemap::kAces), 2);
}
