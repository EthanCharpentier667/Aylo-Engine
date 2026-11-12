/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TextureCalls
*/

#include "plugins/graphics/TextureCalls.hpp"

#include <array>
#include <filesystem>
#include <string>
#include <string_view>

#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "plugins/graphics/TextureAssets.hpp"

namespace aylo::plugins::graphics {
namespace {
TextureResult loadTexture(ecs::ResMut<TextureAssets> textures, const LoadTextureArguments& arguments) {
    const contract::renderer::TextureDesc desc{
        .filter =
            arguments.linear ? contract::renderer::TextureFilter::kLinear : contract::renderer::TextureFilter::kNearest,
        .repeat = arguments.wrap,
    };
    return TextureResult{.texture = textures->load(std::filesystem::path{arguments.path.view()}, desc)};
}
}  // namespace

void addTextureForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "TextureForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addTextureForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addTextureCalls(ecs::App& app) {
    app.addCall(std::string{kLoadTextureCall}, &loadTexture);
    addTextureForms(app.getPluginRegistry());
}

}  // namespace aylo::plugins::graphics
