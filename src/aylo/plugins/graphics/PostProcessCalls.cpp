/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PostProcessCalls
*/

#include "plugins/graphics/PostProcessCalls.hpp"

#include <format>
#include <string>
#include <string_view>

#include "aylo/contract/renderer/postprocess/PostProcess.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "plugins/graphics/GraphicsResources.hpp"
#include "plugins/graphics/exceptions/PostProcessCallException.hpp"

namespace aylo::plugins::graphics {
namespace {
/// @return The tonemap named @p name.
/// @throws exceptions::PostProcessCallException If no tonemap has this name.
contract::renderer::Tonemap requireTonemap(const std::string_view name) {
    if (name == "none") {
        return contract::renderer::Tonemap::kNone;
    }
    if (name == "reinhard") {
        return contract::renderer::Tonemap::kReinhard;
    }
    if (name == "aces") {
        return contract::renderer::Tonemap::kAces;
    }
    throw exceptions::PostProcessCallException{
        std::format("Unknown tonemap '{}' (expected 'none', 'reinhard' or 'aces')", name)};
}

void setPostProcess(ecs::ResMut<PostProcess> postProcess, const SetPostProcessArguments& arguments) {
    postProcess->settings = toPostProcessSettings(arguments);
}
}  // namespace

contract::renderer::PostProcessSettings toPostProcessSettings(const SetPostProcessArguments& arguments) {
    return contract::renderer::PostProcessSettings{
        .bloom = {.enabled = arguments.bloom,
                  .threshold = arguments.bloomThreshold,
                  .softKnee = arguments.bloomKnee,
                  .intensity = arguments.bloomIntensity},
        .tonemap = requireTonemap(arguments.tonemap.view()),
        .exposure = arguments.exposure,
    }
        .sanitized();
}

void addPostProcessCalls(ecs::App& app) { app.addCall(std::string{kSetPostProcessCall}, &setPostProcess); }

}  // namespace aylo::plugins::graphics
