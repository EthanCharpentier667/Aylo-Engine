/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScreenEffect
*/

#include "aylo/contract/renderer/shaders/ScreenEffect.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace aylo::contract::renderer {

std::optional<std::string> checkScreenEffects(std::span<const ScreenEffectPass> passes) {
    std::vector<std::string> written;
    std::size_t index = 0;
    for (const ScreenEffectPass& pass : passes) {
        ++index;
        const auto known = [&written](const std::string& target) {
            return target == kSceneTarget || std::ranges::find(written, target) != written.end();
        };
        if (!pass.input.empty() && !known(pass.input)) {
            return std::format("pass {} reads '{}', which no pass before it writes", index, pass.input);
        }
        for (const ScreenEffectInput& input : pass.inputs) {
            if (!known(input.target)) {
                return std::format("pass {} reads '{}', which no pass before it writes", index, input.target);
            }
        }
        if (pass.output == kSceneTarget) {
            return std::format("pass {} writes '{}', the picture before the effects", index, kSceneTarget);
        }
        if (!pass.output.empty() && std::ranges::find(written, pass.output) == written.end()) {
            written.push_back(pass.output);
        }
    }
    return std::nullopt;
}

}  // namespace aylo::contract::renderer
