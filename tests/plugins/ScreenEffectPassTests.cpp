/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScreenEffectPassTests
*/

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "aylo/contract/renderer/shaders/ScreenEffect.hpp"

using aylo::contract::renderer::checkScreenEffects;
using aylo::contract::renderer::ScreenEffectInput;
using aylo::contract::renderer::ScreenEffectPass;

TEST(ScreenEffectPass, AChainAndAHalfSizeBlurRun) {
    // The issue's bloom: a bright pass, two blurs at half size, then a composite reading the last blur.
    const std::vector<ScreenEffectPass> passes{
        {.output = "bright", .scale = 0.5F},
        {.input = "bright", .output = "blurA", .scale = 0.5F},
        {.input = "blurA", .output = "blurB", .scale = 0.5F},
        {.inputs = {ScreenEffectInput{.slot = 0, .target = "blurB"}}},
    };
    EXPECT_EQ(checkScreenEffects(passes), std::nullopt);
    EXPECT_EQ(checkScreenEffects(std::vector<ScreenEffectPass>{{}, {}}), std::nullopt);
}

TEST(ScreenEffectPass, ReadingTheSceneIsAlwaysAllowed) {
    const std::vector<ScreenEffectPass> passes{{.input = "scene"}, {.inputs = {ScreenEffectInput{.target = "scene"}}}};
    EXPECT_EQ(checkScreenEffects(passes), std::nullopt);
}

TEST(ScreenEffectPass, ATargetNotYetWrittenIsRefused) {
    const std::optional<std::string> later =
        checkScreenEffects(std::vector<ScreenEffectPass>{{.input = "blur"}, {.output = "blur"}});
    ASSERT_TRUE(later);
    EXPECT_TRUE(later->contains("pass 1 reads 'blur'")) << *later;
    const std::optional<std::string> own = checkScreenEffects(
        std::vector<ScreenEffectPass>{{.inputs = {ScreenEffectInput{.target = "self"}}, .output = "self"}});
    ASSERT_TRUE(own);
    EXPECT_TRUE(own->contains("'self'")) << *own;
}

TEST(ScreenEffectPass, TheSceneCannotBeWritten) {
    const std::optional<std::string> problem = checkScreenEffects(std::vector<ScreenEffectPass>{{.output = "scene"}});
    ASSERT_TRUE(problem);
    EXPECT_TRUE(problem->contains("writes 'scene'")) << *problem;
}
